
#include <ncurses.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <functional>
#include <algorithm>

bool exitRequested = false;

std::thread timer_start(std::function<void(void)> func, unsigned int interval)
{
  return std::thread([func, interval]()
  { 
    while (!exitRequested)
    { 
      auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval);
      func();
      std::this_thread::sleep_until(x);
    }
  });
}

int loop = 0;
// We'll put game state here
int curX = 10;
int curY = 10;




void GameLoop(WINDOW *window) {
	// Get input
	char x = getch();

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
			curX = std::max(curX -1, minX+1);
			break;
		case KEY_RIGHT:
			curX = std::min(curX + 1,  maxX-1);
			break;
		case KEY_UP:
			curY = std::max(curY-1, minY+1);
			break;
		case KEY_DOWN:
			curY = std::max(curY+1, maxY-1);
			break;
	}

	// Draw the output 
    box(window, '|', '-');
	mvprintw(10, 10, "The iteration is %d", loop++);
    wrefresh(window); /* Print it on to the real screen */
}

int main() {
  WINDOW *window = initscr(); /* Start curses mode 		  */
  printw("Hello World !!!");  /* Print Hello World		  */
  curs_set(0);
  raw();
  noecho();
  nodelay(stdscr, TRUE);
  timer_start(std::bind(GameLoop,window),100).join();
  
  endwin(); /* End curses mode		  */
  return 0;
}
