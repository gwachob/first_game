
#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <ncurses.h>
#include <thread>
#include <vector>
#define PI 3.14159265

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
struct Point2D {
  int x = 0;
  int y = 0;
};

struct Item {
  char label;
  Point2D center;

  Item(char label, Point2D center) : label(label), center(center){};
};

Point2D rotatePoint(Point2D start, Point2D center, float degrees) {
  float s = sin(degrees * PI / 180);
  float c = cos(degrees * PI / 180);

  Point2D result;
  result.x = start.x - center.x;
  result.y = start.y - center.y;

  result.x = (int)(result.x * c - result.y * s);
  result.y = (int)(result.x * s + result.y * c);

  result.x += center.x;
  result.y += center.y;

  return result;
}
struct RotatingSquare {
  char label;

  Point2D center;
  int size;
  float rotation;

  RotatingSquare(char label, Point2D center, int size = 1, float rotation = 0.0)
      : label(label), center(center), size(size), rotation(rotation){};
};

template <typename T, class... Args> struct Aged {
  int birthTime;
  T item;
  Aged(T item, int birthTime = 0)
      : birthTime(birthTime), item(std::move(item)){};
};

int loop = 0;
// We'll put game state here
Point2D cursor{10, 10};
int timeSpeed = 1;

template <class T> class AgedVector {
  std::vector<Aged<T>> agedItems;
  int age;
  std::function<bool(const Aged<T> &, int)> filterFunction = nullptr;

public:
  AgedVector<T>(
      int age = 0,
      std::function<bool(const Aged<T> &, int)> filterFunction = nullptr)
      : age(age), filterFunction(filterFunction){};

  void add(T item) { agedItems.push_back(Aged<T>(item, age)); };

  void tick(int increment = 1) {
    age += increment;
    if (filterFunction != nullptr) {
      auto i = std::begin(agedItems);
      while (i != std::end(agedItems)) {
        if (!filterFunction(*i, age)) {
          i = agedItems.erase(i);
        } else {
          ++i;
        }
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

AgedVector<Item> timeoutList(0, under100Ticks);
AgedVector<RotatingSquare> squares;

void addDecaying(WINDOW *window, Point2D center) {
  timeoutList.add(Item('*', center));
}

void addSquare(WINDOW *window, Point2D center) {
  squares.add(RotatingSquare('.', center));
}

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
    cursor.x = std::max(cursor.x - 1, minX + 1);
    break;
  case KEY_RIGHT:
    cursor.x = std::min(cursor.x + 1, maxX - 2);
    break;
  case KEY_UP:
    cursor.y = std::max(cursor.y - 1, minY + 1);
    break;
  case KEY_DOWN:
    cursor.y = std::min(cursor.y + 1, maxY - 2);
    break;
  case '+':
    timeSpeed++;
    if (timeSpeed > 100) {
      timeSpeed = 100;
    }
    break;
  case '-':
    timeSpeed--;
    if (timeSpeed < 1) {
      timeSpeed = 1;
    }
    break;
  case 's':
    addSquare(window, cursor);
    break;
  case ' ':
    addDecaying(window, cursor);
    break;
  }

  // Draw the output
  werase(window);
  wborder(window, 0, 0, 0, 0, 0, 0, 0, 0);
  mvprintw(10, 10, "The iteration is %d", loop++);
  mvprintw(11, 10, "The time multiplier is %d", timeSpeed);

  timeoutList.tick(timeSpeed);
  for (const auto &item : timeoutList) {
    mvwaddch(window, item.item.center.y, item.item.center.x,
             item.item.label |
                 COLOR_PAIR((timeoutList.getAge() - item.birthTime) / 10 + 50));
  }

  for (const auto &agedSquare : squares) {
    const RotatingSquare &square = agedSquare.item;
    int leftX = square.center.x - square.size;
    int rightX = square.center.x + square.size;
    int topY = square.center.y - square.size;
    int bottomY = square.center.y + square.size;

    // now translate those points by the "rotation" in degrees
    // (upperLeft, uppeRight)

    // draw 4 lines - kinda wasteful, but simple
    // we can do this in two simple loops (vertical lines and horizontal lines)

    //
    for (int x = leftX; x <= rightX; x++) {
      mvwaddch(window, topY, x, square.label);
      mvwaddch(window, bottomY, x, square.label);
    }

    for (int y = topY; y <= bottomY; y++) {
      mvwaddch(window, y, leftX, square.label);
      mvwaddch(window, y, rightX, square.label);
    }
  }

  mvwaddch(window, cursor.y, cursor.x, 'X' | A_BOLD);
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
