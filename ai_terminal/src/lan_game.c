#include "lan_game.h"

#include <stdio.h>

int lan_game_init(const char *host, int port)
{
    (void)host;
    (void)port;
    printf("[lan_game] 联机功能未实现\n");
    fflush(stdout);
    return -1;
}

int lan_game_host(int port)
{
    (void)port;
    printf("[lan_game] 联机功能未实现\n");
    fflush(stdout);
    return -1;
}

int lan_game_join(const char *host, int port)
{
    (void)host;
    (void)port;
    printf("[lan_game] 联机功能未实现\n");
    fflush(stdout);
    return -1;
}

int lan_game_leave(void)
{
    printf("[lan_game] 联机功能未实现\n");
    fflush(stdout);
    return -1;
}

int lan_game_send_paddle(int y)
{
    (void)y;
    printf("[lan_game] 联机功能未实现\n");
    fflush(stdout);
    return -1;
}
