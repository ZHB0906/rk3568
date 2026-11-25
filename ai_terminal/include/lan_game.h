#ifndef AI_TERMINAL_LAN_GAME_H
#define AI_TERMINAL_LAN_GAME_H

/* 联机乒乓：本次只保留接口，全部为占位实现（返回 -1）。 */

int lan_game_init(const char *host, int port);
int lan_game_host(int port);
int lan_game_join(const char *host, int port);
int lan_game_leave(void);
int lan_game_send_paddle(int y);

#endif /* AI_TERMINAL_LAN_GAME_H */
