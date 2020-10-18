
#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <ncurses.h>
#include <thread>
#include <vector>

bool exitRequested = false;

std::thread timer_start(std::function<void(void)> func, unsigned int interval) {
  return std::thread([func, interval]() {
    while (!exitRequested) {
      auto x = std::chrono::steady_clock::now() +
               std::chrono::milliseconds(interval);
      func();
      std::this_thread::sleep_until(x);
    }
  });
}
struct Item {
  char label;
  int y;
  int x;

  Item(char label, int y, int x) : label(label), y(y), x(x){};
};

int loop = 0;
// We'll put game state here
int curX = 10;
int curY = 10;
std::vector<Item> items;

void doBomb(WINDOW *window, int y, int x) { items.emplace_back('*', y, x); }

void GameLoop(WINDOW *window) {
  // Get input
  int x = getch();

  // Update the model
  int minX = getbegx(window);
  int minY = getbegy(window);
  int maxX = getmaxx(window);
  int maxY = getmaxy(window);

  switch (x) {
  case 'x':
    exitRequested = true;
    return;
    break;
  case KEY_LEFT:
    curX = std::max(curX - 1, minX + 1);
    break;
  case KEY_RIGHT:
    curX = std::min(curX + 1, maxX - 2);
    break;
  case KEY_UP:
    curY = std::max(curY - 1, minY + 1);
    break;
  case KEY_DOWN:
    curY = std::min(curY + 1, maxY - 2);
    break;
  case ' ':
    doBomb(window, curY, curX);
    break;
  }

  // Draw the output
  wclear(window);
  box(window, '|', '-');
  mvprintw(10, 10, "The iteration is %d", loop++);
  for (const auto &item : items) {
    mvwaddch(window, item.y, item.x, item.label);
  }
  mvwaddch(window, curY, curX, 'X' | A_BOLD);
  wrefresh(window); /* Print it on to the real screen */
}

int main() {
  WINDOW *window = initscr(); /* Start curses mode 		  */
  curs_set(0);
  raw();
  noecho();
  cbreak();
  nodelay(stdscr, TRUE);
  keypad(stdscr, TRUE);
  wresize(window, 50, 50);
  timer_start(std::bind(GameLoop, window), 50).join();

  endwin(); /* End curses mode		  */
  return 0;
}
