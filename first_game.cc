
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

Point2D rotatePoint(Point2D start, Point2D center, int degrees) {

  float s = sin(degrees * PI / 180.0);
  float c = cos(degrees * PI / 180.0);

  float resultX = start.x - center.x;
  float resultY = start.y - center.y;

  // Rotation matrix is calculated here
  float dx = resultX * c - resultY * s;
  float dy = resultX * s + resultY * c;

  return Point2D{int(center.x + dx), int(center.y + dy)};
}

void drawLine(Point2D start, Point2D end, WINDOW *window, char symbol) {
  // Using Bresenham's line algorithm from
  // https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
  Point2D current = start;
  int dx = abs(end.x - start.x);
  int sx = start.x < end.x ? 1 : -1;

  int dy = -abs(end.y - start.y);
  int sy = start.y < end.y ? 1 : -1;

  int err = dx + dy;

  while (true) {
    mvwaddch(window, current.y, current.x, symbol);

    if ((current.x == end.x) && (current.y == end.y))
      break;

    int e2 = 2 * err;

    if (e2 >= dy) {
      err += dy;
      current.x += sx;
    }

    if (e2 < dx) {
      err += dx;
      current.y += sy;
    }
  }
}

struct RotatingSquare {
  char label;

  Point2D center;
  int size;
  float rotationSpeed;

  RotatingSquare(char label, Point2D center, int size = 1,
                 float rotationSpeed = 0.0)
      : label(label), center(center), size(size),
        rotationSpeed(rotationSpeed){};
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
  squares.add(RotatingSquare('.', center, 2, 10.0));
}

void drawSquare(WINDOW *window, const RotatingSquare& square, int angle) {
  int leftX = square.center.x - square.size;
    int rightX = square.center.x + square.size;
    int topY = square.center.y - square.size;
    int bottomY = square.center.y + square.size;

    // I don't like that the calculation of the rotation angle here has to use
    // timeSpeed again, when we already passed it in to the squares via tick()

    // Maybe we shouldn't bother with a AgedVector for boxes? Or augment it?

    // Calculate rotated/translated
    Point2D upperLeft = rotatePoint(
        Point2D{leftX, topY}, square.center, angle);
    Point2D upperRight = rotatePoint(
        Point2D{rightX, topY}, square.center, angle);
    Point2D lowerLeft = rotatePoint(
        Point2D{leftX, bottomY}, square.center, angle);
    Point2D lowerRight = rotatePoint(
        Point2D{rightX, bottomY}, square.center, angle);

    // Now draw 4 lines
    drawLine(upperLeft, upperRight, window, square.label);
    drawLine(upperLeft, lowerLeft, window, square.label);
    drawLine(upperRight, lowerRight, window, square.label);
    drawLine(lowerLeft, lowerRight, window, square.label);
    mvwaddch(window, upperLeft.y, upperLeft.x, '+');
    mvwaddch(window, upperRight.y, upperRight.x, '+');
    mvwaddch(window, lowerLeft.y, lowerLeft.x, '+');
    mvwaddch(window, lowerRight.y, lowerRight.x, '+');
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
  squares.tick(timeSpeed);
  for (const auto &item : timeoutList) {
    mvwaddch(window, item.item.center.y, item.item.center.x,
             item.item.label |
                 COLOR_PAIR((timeoutList.getAge() - item.birthTime) / 10 + 50));
  }

  for (const auto &agedSquare : squares) {
    const RotatingSquare &square = agedSquare.item;
    int angle = (square.rotationSpeed * timeSpeed * (loop - agedSquare.birthTime));
    drawSquare(window, square, angle);
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
  wresize(window, 300, 300);
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
