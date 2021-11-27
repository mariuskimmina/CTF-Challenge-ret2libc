#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

char *addr_of_printf;

void print_banner() {
    printf("   _____                 .__         ___________                   __  .__                ___________.__            .___            \n");
    printf("  /     \\ _____     ____ |__| ____   \\_   _____/_ __  ____   _____/  |_|__| ____   ____   \\_   _____/|__| ____    __| _/___________ \n");
    printf(" /  \\ /  \\__  \\   / ___\\|  |/ ___\\   |    __)|  |  \\/    \\_/ ___\\   __\\  |/  _ \\ /    \\   |    __)  |  |/    \\  / __ |/ __ \\_  __ \\\n");
    printf("/    Y    \\/ __ \\_/ /_/  >  \\  \\___   |     \\ |  |  /   |  \\  \\___|  | |  (  <_> )   | \\  |     \\   |  |   |  \\/ /_/ \\  ___/|  | \\/\n");
    printf("\\____|__  (____  /\\___  /|__|\\___  >  \\___  / |____/|___|  /\\___  >__| |__|\\____/|___|  /  \\___  /   |__|___|  /\\____ |\\___  >__|   \n");
    printf("        \\/     \\//_____/         \\/       \\/             \\/     \\/                    \\/       \\/            \\/      \\/    \\/       \n");
}


void printf_finder() {
    char buffer[64];
    puts("found printf, it's at:");
    puts(addr_of_printf);
    puts("Wait, how did you get here?");
    fflush(stdin);
    gets(buffer);
    return;
}

void start_challenge() {
    char buffer[64];
    asprintf(&addr_of_printf, "%p", printf);

    printf("This is a magic function finder!\n");
    printf("Unfortunately, we are still working on this service\n");
    printf("We believe in Open Source, but github is evil, that's why we self host our code on our website\n");
    printf("Maybe you can email us a contribution?\n");
    printf("At the moment only printf_finder has been implemented at: %p\n", printf_finder);
    printf("But we won't let you use it before all our services are ready and available\n");

    printf("If you want, you can give us some feedback: \n");
    gets(buffer);
    return;
}

void main(int argc, char* argv[]) {
    setvbuf(stdout, 0LL, 2, 0LL);
    setvbuf(stdin, 0LL, 1, 0LL);
    print_banner();
    start_challenge();
    fflush(stdout);
    printf("Thank you for your feedback!\n");
}
