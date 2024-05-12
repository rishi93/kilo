/*** includes ***/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/*** data ***/

struct termios orig_termios;

/*** terminal ***/
void die(const char *s) {
    /*
        Most C library functions that fail will set the global errno variable
        to indicate what the error was. perror() looks at the global errno
        variable and prints a descriptive error message for it. It also prints
        the string given to it before it prints the error message, which it
        meant to provide context about what part of your code caused the error. 
    */
    perror(s);
    /*
        After printing out the error message, we exit the program with an exit
        status of 1, which indicate failure (as would any non-zero value). 
    */
    exit(1);
}

void disableRawMode(void) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        die("tcsetattr");
}
/*
    We can set a terminal's attributes by using tcgetattr() to read
    the current attributes into a struct, modifying the struct by hand,
    and passing the modified struct to tcsetattr() to write the new
    terminal attributes back out.
*/
void enableRawMode(void) {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");

    /*
        atexit() comes from <stdlib.h>. We use it to register our
        disableRawMode() function to be called automatically when the program
        exits, whether it exits by returning from main(), or by calling the
        exit() function. This way we can ensure we'll leave the terminal
        attributes the way we found them when our program exits. 
    */
    atexit(disableRawMode);
    /*
        The ECHO feature causes each key you type to be printed to the terminal
        so you can see what you're typing. This is useful in canonical mode,
        but really gets in the way when we are trying to carefully render a
        user interface in raw mode. So we turn it off. 
    */
    // The c_lflag is for "local flags".
    /*
        A comment in macOS's <termios.h> describes it as a "dumping ground
        for other state". So perhaps it should be thought of as "miscellaneous
        flags". The other flag fields are c_iflag (input flags), c_oflag 
        (output flags) and c_cflag (control flags), all of which we will have to
        modify to enable raw mode.
    */

    /*
        We assign the original termios struct to the raw struct inorder to
        make a copy of it before we start making our changes. 
    */
    struct termios raw = orig_termios;
    /*
        ECHO is a bitflag, defined as 0000...1000 in binary. When we use the
        bitwise NOT operator it becomes 1111...0111. We then bitwise AND this
        value with the flags field, to force only this bit to become 0, and all
        other bits retain their existing value. Flipping bits like this is
        common in C.
    */
    // raw.c_lflag &= ~(ECHO);
    /*
        There is an ICANON flag that allows us to turn off canonical mode. This
        means we will finally be reading input byte by byte, instead of line
        by line. 
    */
    /*
        By default, Ctrl+C sends a SIGINT signal to the current process, which
        causes it to terminate, and Ctrl+Z sends a SIGTSTP signal to the current
        process, which causes it to suspend. We turn off both these signals.
    */
    /*
        IXON comes from <termios.h>. The I stands for Input flag, and XON comes
        from the names of the two control characters, that Ctrl+S and Ctrl+Q
        produce. XOFF to pause transmission and XON to resume transmission. 
    */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_iflag &= ~(OPOST);
    raw.c_iflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /*
        VMIN value sets the minimum number of bytes of input needed before
        read() can return. We set it to 0, so that read() returns as soon as
        there is any input to be read. The VTIME value sets the maximum amount
        of time to wait before read() returns. It is in tenths of a second, so
        we set it to 1/10 of a second, or 100 milliseconds. If read() times out,
        it will return 0 (the number of bytes read). 
    */
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    /*
        The TCSAFLUSH argument specifies when to apply the change: in this case,
        it waits for all pending output to be written to the terminal, and also
        discards any input that hasn't been read. 
    */
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

/*** init ***/
int main(void) {
    enableRawMode();
    /*
    read and STDIN_FILENO come from unistd.h
    We are asking read to read 1 byte from the standard input
    into the variable c, and to keep doing it until there
    are no more bytes to read. read() returns the number of bytes
    that it read, and will return 0 when it reaches the end of a file.

    When you run ./kilo, your terminal gets hooked up to the standard input,
    and so your keyboard input gets read into the c variable. However, by
    default your terminal starts in canonical mode, also called cooked mode.
    In this mode, keyboard input is only sent to the program when the user
    pressed Enter.
    
    We want to process each keypress as it comes in, so we can respond to it
    immediately. What we want is raw mode.

    Unfortunately, there is no simple switch you can flip to set the terminal
    to raw mode. Raw mode is achieved by turning off a great many flags in the
    terminal.
    */
    // while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
    while(1) {
        char c = '\0';
        if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) die("read");
        /*
            iscntrl() comes from <ctype.h>, and
            printf comes from <stdio.h>.

            iscntrl() tests whether a character is a control character.
            Control characters are nonprintable characters that we don't want
            to print to the screen.

            printf can print multiple representations of a byte. %d tells it to
            format the byte as a decimal number (it's ASCII code) and %c tells
            it to write out the byte directly, as a character.

            You'll notice, Arrow keys, Page Up/Down, Home/End all input 3 or
            4 bytes to the terminal. 27, '[' and then one or two characters.
            This is known as escape sequence. All escape sequences start with
            a 27 byte. Pressing Esc sends a single 27 byte as input.
        */
        if (iscntrl(c)) {
            printf("%d\r\n", c);
        } else {
            printf("%d ('%c')\r\n", c, c);
        }

        if (c == 'q') break;
    }

    return 0;
}
