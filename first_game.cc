
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

template <typename T, class... Args> struct Aged {
  int birthTime;
  T item;
  Aged(T item, int birthTime = 0)
      : birthTime(birthTime), item(std::move(item)){};
};

int loop = 0;
// We'll put game state here
int curX = 10;
int curY = 10;
template <class T> class TimeoutList {
  std::vector<Aged<T>> agedItems;
  int age;
  std::function<bool(const Aged<T> &, int)> filterFunction = nullptr;

public:
  TimeoutList<T>(
      int age = 0,
      std::function<bool(const Aged<T> &, int)> filterFunction = nullptr)
      : age(age), filterFunction(filterFunction){};

  void add(T item) { agedItems.push_back(Aged<T>(item, age)); };

  void tick(int increment=1) {
    age += increment;
    auto i = std::begin(agedItems);
    while (i != std::end(agedItems)) {
      if (!filterFunction(*i, age)) {
        i = agedItems.erase(i);
      } else {
        ++i;
      }
    }
  }

  auto begin() const { return agedItems.begin(); }
  auto end() const { return agedItems.end(); }
  int getAge() { return age; }
};

bool under100Ticks(const Aged<Item> &agedItem, int time) {
  if ((time - agedItem.birthTime) > 100) {
    return false;
  }
  return true;
}

TimeoutList<Item> timeoutList(0, under100Ticks);

void doBomb(WINDOW *window, int y, int x) { timeoutList.add(Item('*', y, x)); }

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
  werase(window);
  wborder(window, 0, 0, 0, 0, 0, 0, 0, 0);
  mvprintw(10, 10, "The iteration is %d", loop++);

  timeoutList.tick();
  for (const auto &item : timeoutList) {
    mvwaddch(window, item.item.y, item.item.x,
             item.item.label |
                 COLOR_PAIR((timeoutList.getAge() - item.birthTime) / 10 + 50));
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
