// Brain Fart

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define PCELL_START 40

#define TICKS 25
#define ANIM_TICKS 10

#define pwidth 12
#define pheight 22

#ifdef __unix__

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <sys/time.h>
#include <unistd.h>

#define SPF (1E6 / 60)

#define minwidth pwidth * 20
#define minheight pheight * 20

Display *display;
Window  window;
GC	gc;
XEvent	event;
Atom	closing;

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


#elif defined(_WIN32) || defined(WIN32) || defined(__WIN32)

#include <windows.h>
#define SPF (1.0 / 60)

#else
#error "NOT IMPLEMENTED SORRY!"
#endif

int width = PCELL_START * pwidth;
int height = PCELL_START * pheight;

int iframes = 0; // invincibility frame bois
int pausing = 0;

double pcell = 30;
double pborder = 0.1;

int game_over = 0;
int score = 0;

char lines_to_be_destroyed[pheight] = {0};
int lines_to_be_destroyed_len = 0;

char buffer[pwidth * pheight] = {0};

enum tile_colors
{
	GLASS = 0,
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
	ORANGE, GLASS, GLASS,
	ORANGE, GLASS, GLASS,
	ORANGE, ORANGE,      GLASS
} };

const struct tetromino_t LEFT_L  = { .x = 5, .y = 1, .ticks = 0, .move = 0, .dim = 3, .test = {
	GLASS, BLUE, GLASS,
	GLASS, BLUE, GLASS,
	BLUE, BLUE, GLASS
} };

const struct tetromino_t FIVE    = { .x = 5, .y = 1, .ticks = 0, .move = 0, .dim = 3, .test = {
	GLASS, GREEN, GREEN,
	GREEN, GREEN, GLASS,
	GLASS, GLASS, GLASS
} };

const struct tetromino_t RFIVE   = { .x = 5, .y = 1, .ticks = 0, .move = 0, .dim = 3, .test = {
	RED, RED, GLASS,
	GLASS, RED, RED,
	GLASS, GLASS, GLASS
} };

const struct tetromino_t LEDGE   = { .x = 5, .y = 1, .ticks = 0, .move = 0, .dim = 3, .test = {
	PURPLE, PURPLE, PURPLE,
	GLASS, PURPLE, GLASS,
	GLASS, GLASS, GLASS
} };

const struct tetromino_t LINE = { .x = 5, .y = 1, .ticks = 0, .move = 0, .dim = 4, .test = {
	CYAN, GLASS, GLASS, GLASS,
	CYAN, GLASS, GLASS, GLASS,
	CYAN, GLASS, GLASS, GLASS,
	CYAN, GLASS, GLASS, GLASS
} };

#ifdef __unix__
void draw_tile(int tile, int j, int i)
{
	double border = pborder * pcell;
	switch(tile)
	{
		case GLASS:
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

#elif defined(_WIN32) || defined(WIN32) || defined(__WIN32)

void draw_tile(HDC hdc, int tile, int j, int i)
{
	double border = pborder * pcell;
	switch(tile)
	{
		case GLASS:
			// Do Nothing
			break;

		case CYAN:   SetDCBrushColor(hdc, RGB(0x00, 0xff, 0xff)); goto DRAW;
		case BLUE:   SetDCBrushColor(hdc, RGB(0x00, 0x00, 0xff)); goto DRAW;
		case ORANGE: SetDCBrushColor(hdc, RGB(0xaa, 0xaa, 0x00)); goto DRAW;
		case YELLOW: SetDCBrushColor(hdc, RGB(0xff, 0xff, 0x00)); goto DRAW;
		case GREEN:  SetDCBrushColor(hdc, RGB(0x00, 0xff, 0xf0)); goto DRAW;
		case PURPLE: SetDCBrushColor(hdc, RGB(0xf0, 0x00, 0xff)); goto DRAW;
		case RED:    SetDCBrushColor(hdc, RGB(0xfa, 0x00, 0x00)); goto DRAW;

		case BRICK:  
			     // border = 0;
			     SetDCBrushColor(hdc, RGB(0xaa, 0x4a, 0x44));
			     goto DRAW;

		case DEATH_ANIM:  
			     border = 0;
			     if(iframes < ANIM_TICKS / 3)
				     SetDCBrushColor(hdc, RGB(0xff, 0x0a, 0xff));
			     else if(iframes <= ANIM_TICKS * 2 / 3)
				     SetDCBrushColor(hdc, RGB(0x00, 0xff, 0x00));
			     else if(iframes < ANIM_TICKS)
				     SetDCBrushColor(hdc, RGB(0xf0, 0xf0, 0xf0));

DRAW:
			     {
				     int x = j * pcell + border;
				     int y = i * pcell + border;
				     int w = pcell - (j == pwidth - 1 ? 1.5 : 1) * border;
				     int h = pcell - (i == pheight - 1 ? 1.5 : 1) * border;

				     SelectObject(hdc, GetStockObject(DC_BRUSH));
				     Rectangle(hdc, x, y, x + w, y + h);
			     }
			     break;
	}
}
#else
#error "NOT IMPLEMENTED SORRY!"
#endif

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

			if(tetromino.test[i * tetromino.dim + j] != GLASS &&
					buffer[(i + tetromino.y) * pwidth + j + tetromino.x] != GLASS)
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
			if(buffer[(i + tetromino.y) * pwidth + j + tetromino.x] != GLASS) continue;
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

#ifdef __unix__

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

#elif defined(_WIN32) || defined(WIN32) || defined(__WIN32)

HDC device_context = 0;

void update_game()
{
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
}


void render_game(HWND window)
{
	PatBlt(device_context, 0, 0, width, height, BLACKNESS);

	if(!iframes)
	{
		for(int i = 0; i < tetromino.dim; i++)
		{
			for(int j = 0; j < tetromino.dim; j++)
			{
				draw_tile(device_context, tetromino.test[i * tetromino.dim + j], j + tetromino.x, i + tetromino.y);
			}
		}
	}

	for(int i = 0; i < pheight; i++)
	{
		for(int j = 0; j < pwidth; j++)
		{
			draw_tile(device_context, buffer[i * pwidth + j], j, i);
		}
	}

	HDC defdc = GetDC(window);
	StretchBlt(defdc, 0, 0, width, height, device_context, 0, 0, width, height, SRCCOPY);
	ReleaseDC(window, defdc);
}

LRESULT WndProc(HWND window, UINT msg, WPARAM wp, LPARAM lp)
{
	switch(msg)
	{
		case WM_GETMINMAXINFO:
			{
				MINMAXINFO *mmi = (MINMAXINFO *)lp;

				RECT wr, cr;

				GetWindowRect(window, &wr);
				GetClientRect(window, &cr);

				int wid = (wr.right - wr.left) - (cr.right - cr.left) + width;
				int hei = (wr.bottom - wr.top) - (cr.bottom - cr.top) + height;

				mmi->ptMinTrackSize.x = wid; mmi->ptMinTrackSize.y = hei;
				mmi->ptMaxTrackSize.x = wid; mmi->ptMaxTrackSize.y = hei;
			}

			return 0;

		case WM_KEYDOWN:
			if(GetAsyncKeyState(VK_UP))
			{
				rotate_tetromino();
			}
			if(GetAsyncKeyState(VK_DOWN))
			{
				tetromino.move = 3;
			}
			if(GetAsyncKeyState(VK_LEFT))
			{
				tetromino.move = 1;
			}
			if(GetAsyncKeyState(VK_RIGHT))
			{
				tetromino.move = 2;
			}
			if(GetAsyncKeyState(VK_SPACE))
			{
				pausing = !pausing;
			}
			if(GetAsyncKeyState(VK_ESCAPE))
			{
				game_over = 1;
			}
			return 0;

		case WM_KEYUP:
			tetromino.move = 0;
			return 0;

		case WM_CLOSE:
			game_over = 1;
			PostQuitMessage(0);
			return 0;
	}

	return DefWindowProc(window, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmdStr, int nCmdShow)
{
	srand(time(NULL));

	WNDCLASS wndcls = {0};
	wndcls.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wndcls.lpfnWndProc = WndProc;
	wndcls.hInstance = hInst;
	wndcls.hIcon = LoadIcon(hInst, IDI_APPLICATION);
	wndcls.hCursor = LoadIcon(hInst, IDC_ARROW);
	wndcls.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	wndcls.lpszClassName = "WOTRIS";

	LARGE_INTEGER freq, time_a, time_b;

	QueryPerformanceFrequency(&freq);

	RegisterClass(&wndcls);

	HWND window = CreateWindow("WOTRIS", "WOTRIS",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			width, height,
			NULL, NULL, hInst, NULL);

	// Creating DC
	HDC defdc = GetDC(window);
	device_context = CreateCompatibleDC(defdc);
	ReleaseDC(window, defdc);

	BITMAPINFOHEADER bmih = {
		.biSize = sizeof(BITMAPINFOHEADER),
		.biWidth = width,
		.biHeight = height,
		.biPlanes = 1,
		.biBitCount = 32,
		.biCompression = BI_RGB,
		.biSizeImage = 0
	};

	BITMAPINFO bmif = {bmih};

	void *raw_pixel_buffer; // Not Used
	HBITMAP hbm = CreateDIBSection(device_context, &bmif, DIB_RGB_COLORS, &raw_pixel_buffer, NULL, 0);
	SelectObject(device_context, hbm);

	memset(buffer, BRICK, pwidth);
	memset(buffer + (pheight - 1) * pwidth, BRICK, pwidth);

	for(int i = 0; i < pheight; i++)
	{
		buffer[i * pwidth] = BRICK;
		buffer[i * pwidth + pwidth - 1] = BRICK;
	}

	ShowWindow(window, nCmdShow);

	while(!game_over)
	{
		QueryPerformanceCounter(&time_a);

		MSG message;
		while(PeekMessage(&message, window, 0, 0, PM_REMOVE))
		{
			if(message.message == WM_QUIT)
				game_over = 1;

			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		update_game();
		render_game(window);

		double elapsed_time;

		QueryPerformanceCounter(&time_b);
		elapsed_time = (double)(time_b.QuadPart - time_a.QuadPart) / freq.QuadPart;

		int sleep_time = (SPF - elapsed_time) * 1000;

		if(sleep_time <= 0)
			puts("DROPPING FRAMES!!");
		else
			Sleep(sleep_time);
	}

	static char score_buffer[255];

	sprintf(score_buffer, "SCORE = %d!!!", score);	

	MessageBox(window, score_buffer, "WOTRIS SCORE!!!", MB_OK);

	DeleteDC(device_context);
	DestroyWindow(window);
	UnregisterClass("WOTRIS", hInst);

	return 0;
}

#else
#error "NOT IMPLEMENTED SORRY!"
#endif
