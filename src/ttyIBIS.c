/*
    ttyIBIS - A linux tool to send IBIS messages
    Copyright (C) 2021  Colaholiker

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
//---------------------------------=[Includes]=---------------------------------
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <malloc.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>


//-------------------------------=[Static Data]=--------------------------------
static char _ibis_chars[] = 
{
    // Pairs of ASCII code and IBIS replacement character
    0xC4, '[', // Ä
    0xE4, '{', // ä
    0xD6, '\\', // Ö
    0xF6, '|', // ö
    0xDC, ']', // Ü
    0xFC, '}', // ü
    0xDF, '~' // ß
};

//--------------------------------=[Prototypes]=--------------------------------
static uint8_t _calculate_checksum(const char* telegram);
static void _ibis_encode(char* message);
static void _process_command_line(char* message);
static void _replace_char(char* str, char find, char replace);

//-----------------------------------=[Code]=-----------------------------------
int main(int argc, char* argv[])
{
    char cr = '\r';
    int serial_port; // Serial port handler
    ssize_t bytes_sent;
    struct termios tty; // For IBIS compatible serial settings
    uint8_t checksum;

    // Make sure locale is set to convert 8-bit characters from cmd line
    setlocale(LC_ALL, "");

    // Handle --version argument
    if ((argc > 1) && (0 == strncmp("--version", argv[1], 10)))
    {
        printf("\nttyIBIS v1.0\nCopyright (C) 2021 Colaholiker\n");
        printf("This is free software; see the source for copying conditions.  There is NO\n");
        printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
        return EXIT_SUCCESS;
    }

    // Print help message when not called correctly
    if (3 != argc)
    {
        // Strip path from executable name
        char* execname_start = strrchr(argv[0], '/');
        if (NULL == execname_start)
        {
            execname_start = argv[0];
        }
        else
        {
            execname_start++;
        }
        printf("Usage: %s [--version] /dev/ttyX Message\nExample: %s /dev/ttyUSB0 l001\n", execname_start, execname_start);
        return EXIT_FAILURE;
    }

    // Convert wide characters and _* combinations
    _process_command_line(argv[2]);
    
    // Apply IBIS specific encoding
    _ibis_encode(argv[2]);

    // Calculate checksum
    checksum = _calculate_checksum(argv[2]);

    // Ready to send - try to open serial port
    serial_port = open(argv[1], O_RDWR | O_NOCTTY);
    if (0 > serial_port)
    {
        fprintf(stderr, "Error opening %s: %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    // Get serial port settings to modify
    if(0 != tcgetattr(serial_port, &tty))
    {
        fprintf(stderr,"Error %i from tcgetattr: %s\n", errno, strerror(errno));
        return EXIT_FAILURE;
    }

    // Make settings changes
    cfmakeraw(&tty); // Raw mode
    tty.c_oflag &= ~(ONLCR | OCRNL); // No "intelligent" CR/LF processing
    tty.c_cflag &= ~(CSIZE | CRTSCTS); // No handshake, clear character size
    // 7 bit, 2 stop bits, even parity, receiver on, no modem control
    tty.c_cflag |= CS7 | CSTOPB | CREAD | CLOCAL | PARENB; 
    tty.c_cc[VTIME] = 10; // Read timeouts (for later use)
    tty.c_cc[VMIN] = 10;
    cfsetspeed(&tty, B1200); // 1200 bps

    // Apply settings and flush buffers
    if (0 != (tcsetattr(serial_port, TCSANOW, &tty)))
    {
        fprintf(stderr,"Error %i from tcsetattr: %s\n", errno, strerror(errno));
        return EXIT_FAILURE;
    }
    tcflush(serial_port, TCIOFLUSH);

    // Write data and wait for complete send
    bytes_sent = write(serial_port, argv[2], strlen(argv[2]));
    bytes_sent += write(serial_port, &cr, 1);
    bytes_sent += write(serial_port, &checksum, 1);
    tcdrain(serial_port);
    close(serial_port);
    return (-1 != bytes_sent) ?  EXIT_SUCCESS : EXIT_FAILURE;
}

//----------------------------=[Private Functions]=-----------------------------
/**
 * Calculate the IBIS specific checksum for a prepared telegram
 *
 * @param Pointer to prepared telegram
 * @returns Checksum
 */
static uint8_t _calculate_checksum(const char* telegram)
{
    uint8_t* p = (uint8_t*)telegram;
    uint8_t checksum = 127;

    while ('\0' != *p)
    {
        checksum ^= *p++;
    }

    // Add CR to checksum
    checksum ^= '\r';
    return checksum;
}

/**
 * Encode characters beyond 7 bit ASCII to their IBIS counterparts
 *
 * @param message Pointer to the message that is encoded
 * @note Characters are replaced in the string we point to - but since single
 * bytes are replaced by single bytes, we are on the safe side.
 */
static void _ibis_encode(char* message)
{
    // _ibis_chars contains of pairs a, b, where a is replaced by b.
    for (int i = 0; i < 14; i += 2)
    {
        _replace_char(message, _ibis_chars[i], _ibis_chars[i+1]);
    }
}

/**
 * Process command line to narrow down wide chars.
 * 
 * Special placeholders will also be evaluated:
 * _n turns into a newline
 * _0.._9 and _A to _F turn into IBIShex notation
 * To pass an underscore, enter two underscores side by side
 *
 * @param message Pointer to the message that is encoded
 * @note Characters are replaced in the string we point to - but wide chars
 * are replaced by single bytes, we are on the safe side.
 */
static void _process_command_line(char* message)
{
    char* dst = message;
    size_t len = strlen(message) + 1;     // For malloc size

    // Temporary UTF-8 decoded buffer and read pointer
    wchar_t* buf = malloc(len * sizeof(wchar_t));
    wchar_t* src = buf;

    wchar_t c; // Temporary value for comparison


    // If malloc worked, let's get started
    if (NULL != buf)
    {
        // Ensure everything is clean
        memset(buf, 0, len * sizeof(wchar_t));

        // Convert UTF-8 to wchar_t
        mbstowcs(buf, message, len);

        // Now loop until end of string
        c = *src++;
        do
        {
            if ('_' == c)
            {
                c = *src++;
                switch (c)
                {
                    case '_':
                        *dst++ = '_';
                        break;
                    case 'n':
                        *dst++ = '\n';
                        break;
                    case 'A':
                        *dst++ = ':';
                        break;
                    case 'B':
                        *dst++ = ';';
                        break;
                    case 'C':
                        *dst++ = '<';
                        break;
                    case 'D':
                        *dst++ = '=';
                        break;
                    case 'E':
                        *dst++ = '>';
                        break;
                    case 'F':
                        *dst++ = '?';
                        break;
                    default:
                        if(('0' <= c) && ('9' >= c))
                        {
                            *dst++ = *message - '0';
                        }
                }
            }
            else
            {
                *dst++ = c;
            }
            c = *src++;
        } while ('\0' != c);

        *dst = '\0';
        free(buf); // Free allocated buffer
    }
}

/**
 * Helper to replace characters in string
 *
 * @param str String to modify
 * @param find Character to replace
 * @param replace Character to replace find with
 */
static void _replace_char(char* str, char find, char replace)
{
    char* current_pos = strchr(str, find);

    while (current_pos)
    {
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
}
