#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <sys/time.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

Display *display;
Window  window;
GC	gc;
XEvent	event;
Atom	closing;

#define TICKS 25
#define SPF (1E6/60)

#define ANIM_TICKS 10

#define pwidth 12
#define pheight 22

#define minwidth pwidth * 20
#define minheight pheight * 20

int width = 30 * pwidth;
int height = 30 * pheight;

int iframes = 0; // invincibility frame bois
int pausing = 0;

double pcell = 30;
double pborder = 0.1;

int game_over = 0;
int score = 0;

XSizeHints size_hints = {
	.flags = PAspect | PResizeInc | PMinSize,

	.width_inc = pwidth,
	.height_inc = pheight,

	.min_width = minwidth,
	.min_height = minheight,

	.max_aspect ={
		.x = pwidth,
		.y = pheight
	},

	.min_aspect = {
		.x = pwidth,
		.y = pheight
	}
};

char lines_to_be_destroyed[pheight] = {0};
int lines_to_be_destroyed_len = 0;

char buffer[pwidth * pheight] = {0};

enum tile_colors
{
	TRANSPARENT = 0,
	CYAN, BLUE, ORANGE, YELLOW, GREEN, PURPLE, RED,
	BRICK, DEATH_ANIM
};

struct tetromino_t
{
	int x;
	int y;
	int move;
	int ticks;
	int dim; // dimentions
	char  test[16]; // test area of current tetrominio
} tetromino = {
	.x = 5,
	.y = 1,
	.ticks = 0,
	.move = 0,	// DO NOT MOVE, MOVE LEFT, MOVE RIGHT
	.dim = 2,
	.test = {
		[0] = YELLOW, [1] = YELLOW,
		[2] = YELLOW, [3] = YELLOW,
	}
};

const struct tetromino_t BLOCK   = { .x = 5, .y = 1, .ticks = 0, .move = 0, .dim = 2, .test = {
	YELLOW, YELLOW,
	YELLOW, YELLOW
} };

const struct tetromino_t RIGHT_L = { .x = 5, .y = 1, .ticks = 0, .move = 0, .dim = 3, .test = {
	ORANGE, TRANSPARENT, TRANSPARENT,
	ORANGE, TRANSPARENT, TRANSPARENT,
	ORANGE, ORANGE,      TRANSPARENT
} };

const struct tetromino_t LEFT_L  = { .x = 5, .y = 1, .ticks = 0, .move = 0, .dim = 3, .test = {
	TRANSPARENT, BLUE, TRANSPARENT,
	TRANSPARENT, BLUE, TRANSPARENT,
	BLUE, BLUE, TRANSPARENT
} };

const struct tetromino_t FIVE    = { .x = 5, .y = 1, .ticks = 0, .move = 0, .dim = 3, .test = {
	TRANSPARENT, GREEN, GREEN,
	GREEN, GREEN, TRANSPARENT,
	TRANSPARENT, TRANSPARENT, TRANSPARENT
} };

const struct tetromino_t RFIVE   = { .x = 5, .y = 1, .ticks = 0, .move = 0, .dim = 3, .test = {
	RED, RED, TRANSPARENT,
	TRANSPARENT, RED, RED,
	TRANSPARENT, TRANSPARENT, TRANSPARENT
} };

const struct tetromino_t LEDGE   = { .x = 5, .y = 1, .ticks = 0, .move = 0, .dim = 3, .test = {
	PURPLE, PURPLE, PURPLE,
	TRANSPARENT, PURPLE, TRANSPARENT,
	TRANSPARENT, TRANSPARENT, TRANSPARENT
} };

const struct tetromino_t LINE = { .x = 5, .y = 1, .ticks = 0, .move = 0, .dim = 4, .test = {
	CYAN, TRANSPARENT, TRANSPARENT, TRANSPARENT,
	CYAN, TRANSPARENT, TRANSPARENT, TRANSPARENT,
	CYAN, TRANSPARENT, TRANSPARENT, TRANSPARENT,
	CYAN, TRANSPARENT, TRANSPARENT, TRANSPARENT
} };

void draw_tile(int tile, int j, int i)
{
	double border = pborder * pcell;
	switch(tile)
	{
		case TRANSPARENT:
			// Do Nothing
			break;

		case CYAN:   XSetForeground(display, gc, 0x00ffff); goto DRAW;
		case BLUE:   XSetForeground(display, gc, 0x0000ff); goto DRAW;
		case ORANGE: XSetForeground(display, gc, 0xaaaa00); goto DRAW;
		case YELLOW: XSetForeground(display, gc, 0xffff00); goto DRAW;
		case GREEN:  XSetForeground(display, gc, 0x00fff0); goto DRAW;
		case PURPLE: XSetForeground(display, gc, 0xf000ff); goto DRAW;
		case RED:    XSetForeground(display, gc, 0xfa0000); goto DRAW;

		case BRICK:  
			     // border = 0;
			     XSetForeground(display, gc, 0xaa4a44);
			     goto DRAW;

		case DEATH_ANIM:  
			     border = 0;
			     if(iframes < ANIM_TICKS / 3)
				     XSetForeground(display, gc, 0xff0aff);
			     else if(iframes <= ANIM_TICKS * 2 / 3)
				     XSetForeground(display, gc, 0x00ff00);
			     else if(iframes < ANIM_TICKS)
				     XSetForeground(display, gc, 0xf0f0f0);

DRAW:
			     XFillRectangle(display, window, gc,
					     j * pcell + border, i * pcell + border,
					     pcell - (j == pwidth - 1 ? 1.5 : 1) * border,
					     pcell - (i == pheight - 1 ? 1.5 : 1) * border);
			     break;
	}
}

int check_tetramino_collision()
{
	// Out of bounds detection

	// Check if coliding
	for(int i = 0; i < tetromino.dim; i++)
	{
		for(int j = 0; j < tetromino.dim; j++)
		{
			if(i * tetromino.dim + j >= tetromino.dim * tetromino.dim) continue;
			if((i + tetromino.y) * pwidth + j + tetromino.x >= pwidth * pheight) continue;

			if(tetromino.test[i * tetromino.dim + j] != TRANSPARENT &&
					buffer[(i + tetromino.y) * pwidth + j + tetromino.x] != TRANSPARENT)
				return 1;
		}
	}

	return 0;
}

void copy_tetromino()
{
	// fprintf(stderr, "INFO : copy_tetromino called\n");

	for(int i = 0; i < tetromino.dim; i++)
	{
		for(int j = 0; j < tetromino.dim; j++)
		{
			if((i + tetromino.y) * pwidth + j + tetromino.x > pwidth * pheight) continue;
			if(buffer[(i + tetromino.y) * pwidth + j + tetromino.x] != TRANSPARENT) continue;
			buffer[(i + tetromino.y) * pwidth + j + tetromino.x] = tetromino.test[i * tetromino.dim + j];
		}
	}
}

void rotate_tetromino()
{
	char test[16] = {0};
	int dim = tetromino.dim;
	struct tetromino_t revert = tetromino;

	// Nothing to do
	if(tetromino.dim == 2)
		return;

	if(tetromino.dim == 4)
	{
		if(tetromino.test[0] == CYAN)
		{
			test[4] = CYAN;
			test[5] = CYAN;
			test[6] = CYAN;
			test[7] = CYAN;
		}
		else
		{
			test[0] = CYAN;
			test[4] = CYAN;
			test[8] = CYAN;
			test[12] = CYAN;
		}
	}
	else
	{
		// Mirror Transpose (j, dim - i - 1)
		for(int i = 0; i < tetromino.dim; i++)
		{
			for(int j = 0; j < tetromino.dim; j++)
			{
				test[j * dim + (dim - i - 1)] = tetromino.test[i * dim + j];
			}
		}
	}

	memcpy(tetromino.test, test, sizeof test);

	if(check_tetramino_collision())
		tetromino = revert;
}

void destroy_lines()
{
	lines_to_be_destroyed_len = 0;

	for(int i = 1; i < pheight - 1; i++)
	{
		int count = 0;
		for(int j = 1; j < pwidth - 1; j++)
		{
			if(buffer[i * pwidth + j])
				count++;
		}

		if(count == pwidth - 2)
		{
			iframes = 1;
			lines_to_be_destroyed[lines_to_be_destroyed_len++] = i;
			memset(&buffer[i * pwidth + 1], DEATH_ANIM, pwidth - 2);
		}
	}
}

void create_new_tetramino()
{
	if(check_tetramino_collision())
	{
		tetromino.y -= 1;

		if(tetromino.y == 1)
		{
			game_over = 1;
		}
		else
		{
			copy_tetromino();
			destroy_lines();

			switch(rand() % 7)
			{
				case 0: memcpy(&tetromino, &BLOCK, sizeof BLOCK); break;
				case 1: memcpy(&tetromino, &RIGHT_L, sizeof RIGHT_L); break;
				case 2: memcpy(&tetromino, &LEFT_L, sizeof LEFT_L); break;
				case 3: memcpy(&tetromino, &FIVE, sizeof FIVE); break;
				case 4: memcpy(&tetromino, &RFIVE, sizeof RFIVE); break;
				case 5: memcpy(&tetromino, &LEDGE, sizeof LEDGE); break;
				case 6: memcpy(&tetromino, &LINE, sizeof LINE); break;
			}
		}
	}
}

int main(void)
{
	srand(time(NULL));

	if(!(display = XOpenDisplay(0)))
		return 1;

	unsigned long bp = BlackPixel(display, DefaultScreen(display));

	window = XCreateSimpleWindow(display,
			DefaultRootWindow(display),
			0, 0, width, height, 0, bp, bp);

	gc = XCreateGC(display, window, 0, 0);

	XStoreName(display, window, "TETRIS");

	closing = XInternAtom(display, "WM_DELETE_WINDOW", 1);
	XSetWMProtocols(display, window, &closing, 1);

	XSetWMNormalHints(display, window, &size_hints);

	XSelectInput(display, window, StructureNotifyMask | KeyPressMask | KeyReleaseMask);

	XAutoRepeatOn(display);

	XMapWindow(display, window);
	XSync(display, 0);

	// Initializing thing
	memset(buffer, BRICK, pwidth);
	memset(buffer + (pheight - 1) * pwidth, BRICK, pwidth);

	for(int i = 0; i < pheight; i++)
	{
		buffer[i * pwidth] = BRICK;
		buffer[i * pwidth + pwidth - 1] = BRICK;
	}

	while(!game_over)
	{
		struct timeval time_a, time_b;

		// FIXME: use the monotonic clock api instead of this.
		gettimeofday(&time_a, NULL);

		while(XPending(display))
		{
			XNextEvent(display, &event);

			switch(event.type)
			{
				case ClientMessage:
					if((unsigned long)event.xclient.data.l[0] == closing)
						game_over = 1;
					break;

				case ConfigureNotify:
					width = event.xconfigure.width;
					height = event.xconfigure.height;
					break;

				case MapNotify:
					{
						XWindowAttributes attrib = {0};
						XGetWindowAttributes(display, window, &attrib);

						width = attrib.width;
						height = attrib.height;
					}
					break;

				case KeyPress:
					{
						KeySym sym = XLookupKeysym(&event.xkey, 0);
						switch(sym)
						{
							case XK_Escape:
								game_over = 1;
								break;

							case XK_Left:
								tetromino.move = 1;
								break;

							case XK_Right:
								tetromino.move = 2;
								break;

							case XK_Up:
								rotate_tetromino();
								break;

							case XK_Down:
								tetromino.move = 3;
								break;

							case XK_space:
								pausing = !pausing;
								break;
						}
					}
					break;

				case KeyRelease:
					tetromino.move = 0;
					break;
			}
		}
		
		// Updating
		pcell = width / pwidth;

		if(!pausing)
		{
			if(iframes == 0)
			{
				if(tetromino.ticks >= TICKS)
				{
					tetromino.y += 1;
					create_new_tetramino();
					tetromino.ticks = 0;
				}
				else if(tetromino.move && tetromino.y > 0)
				{
					switch(tetromino.move)
					{
						case 1:
							tetromino.x -= 1;

							if(check_tetramino_collision())
								tetromino.x += 1;

							tetromino.move = 0;
							break;

						case 2:
							tetromino.x += 1;

							if(check_tetramino_collision())
								tetromino.x -= 1;

							tetromino.move = 0;
							break;

						case 3:
							tetromino.y += 1;
							create_new_tetramino();
							break;
					}
				}

				tetromino.ticks++;
			}
			else
			{
				if(iframes == ANIM_TICKS)
				{
					for(int i = 0; i < lines_to_be_destroyed_len; i++)
					{
						memmove(buffer + 2 * pwidth, buffer + pwidth, lines_to_be_destroyed[i] * pwidth - pwidth);
						memset(buffer + pwidth + 1, 0, pwidth - 2);
						score += pwidth;
					}
					iframes = 0;
				}
				else
				{
					iframes++;
				}
			}
		}

		// Rendering
		XClearWindow(display, window);

		if(!iframes)
		{
			for(int i = 0; i < tetromino.dim; i++)
			{
				for(int j = 0; j < tetromino.dim; j++)
				{
					draw_tile(tetromino.test[i * tetromino.dim + j], j + tetromino.x, i + tetromino.y);
				}
			}
		}

		for(int i = 0; i < pheight; i++)
		{
			for(int j = 0; j < pwidth; j++)
			{
				draw_tile(buffer[i * pwidth + j], j, i);
			}
		}

		XFlush(display);

		gettimeofday(&time_b, NULL);
		suseconds_t dt = time_b.tv_usec - time_a.tv_usec;
		suseconds_t drop = SPF - dt;

		// fprintf(stderr, "INFO: frame_time = %lf, delay needed = %lf\n", 1.0 * 1E-6 * dt, 1.0 * 1E-6 * drop);

		if(drop > 0 && dt > 0) usleep(drop);
		else if(drop < 0) fprintf(stderr, "DROPPING FRAMES!!!\n");
	}

	fprintf(stdout, "SCORE = %d\n", score);

	XCloseDisplay(display);
}
