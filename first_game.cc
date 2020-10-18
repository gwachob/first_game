
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
  int age;

  Item(char label, int y, int x) : label(label), y(y), x(x), age(0){};
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
  wborder(window, 0, 0, 0, 0, 0, 0, 0, 0);
  mvprintw(10, 10, "The iteration is %d", loop++);

  // Kind of a hack - iterate through the vector, possibly removing 
  // items if they are too old. In theory, we'd have a way for these
  // items to remove themselves, or a data structure that better 
  // supported this kind of removal (hashtable?)
  auto i = std::begin(items);
  while (i != std::end(items)) {
    if (i->age > 100) {
      i = items.erase(i);
    } else {
      mvwaddch(window, i->y, i->x, i->label | COLOR_PAIR(i->age / 10 + 50));
      i->age++;
      ++i;
    }
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
  start_color();
  for (int i = 50; i < 60; i++) {
    init_color(i, (200 - (i - 50) * 20), (1000 - (i - 50) * 100),
               (100 - (i - 50) * 10));
    init_pair(i, i, COLOR_BLACK);
  }
  timer_start(std::bind(GameLoop, window), 50).join();
  endwin(); /* End curses mode		  */
  return 0;
}
