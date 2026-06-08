#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <windows.h>
#include <float.h>

/* ---------- Холст (Canvas) с автоматическим масштабированием ---------- */
typedef struct Canvas
{
	int width;
	int height;
	char *buffer;
	double min_x, max_x, min_y, max_y;
	int margin;
} Canvas;

Canvas *canvas_create(int width, int height);
void canvas_destroy(Canvas *c);
void canvas_clear(Canvas *c);
void canvas_configure(Canvas *c, double min_x, double max_x, double min_y, double max_y);
void canvas_put_char(Canvas *c, int x, int y, char ch);
void canvas_display(Canvas *c);
void put_pixel(Canvas *c, int x, int y, char ch);

Canvas *canvas_create(int width, int height)
{
	Canvas *c = (Canvas *)malloc(sizeof(Canvas));
	if (!c)
		return NULL;
	c->width = width;
	c->height = height;
	c->buffer = (char *)malloc((width * height + 1) * sizeof(char));
	if (!c->buffer)
	{
		free(c);
		return NULL;
	}
	c->margin = 2;
	c->min_x = c->min_y = -10;
	c->max_x = c->max_y = 10;
	canvas_clear(c);
	return c;
}

void canvas_destroy(Canvas *c)
{
	if (c)
	{
		free(c->buffer);
		free(c);
	}
}

void canvas_clear(Canvas *c)
{
	for (int i = 0; i < c->width * c->height; i++)
		c->buffer[i] = ' ';
	c->buffer[c->width * c->height] = '\0';
}

void canvas_put_char(Canvas *c, int x, int y, char ch)
{
	if (x >= 0 && x < c->width && y >= 0 && y < c->height)
		c->buffer[y * c->width + x] = ch;
}

void canvas_display(Canvas *c)
{
	for (int y = 0; y < c->height; y++)
	{
		for (int x = 0; x < c->width; x++)
			putchar(c->buffer[y * c->width + x]);
		putchar('\n');
	}
}

void canvas_configure(Canvas *c, double min_x, double max_x, double min_y, double max_y)
{
	if (min_x == max_x)
	{
		min_x -= 1;
		max_x += 1;
	}
	if (min_y == max_y)
	{
		min_y -= 1;
		max_y += 1;
	}
	c->min_x = min_x;
	c->max_x = max_x;
	c->min_y = min_y;
	c->max_y = max_y;
}

void put_pixel(Canvas *c, int x, int y, char ch)
{
	int screen_x = c->margin + (int)((double)(x - c->min_x) / (c->max_x - c->min_x) * (c->width - 2 * c->margin));
	int screen_y = c->margin + (int)((double)(c->max_y - y) / (c->max_y - c->min_y) * (c->height - 2 * c->margin));
	if (screen_x >= 0 && screen_x < c->width && screen_y >= 0 && screen_y < c->height)
		c->buffer[screen_y * c->width + screen_x] = ch;
}

void draw_axes(Canvas *c)
{
	for (int x = (int)c->min_x; x <= (int)c->max_x; x++)
		put_pixel(c, x, 0, '-');
	for (int y = (int)c->min_y; y <= (int)c->max_y; y++)
		put_pixel(c, 0, y, '|');
	put_pixel(c, 0, 0, '+');
	put_pixel(c, (int)c->max_x, 0, 'X');
	put_pixel(c, 0, (int)c->max_y, 'Y');
}

/* ---------- Базовый класс Shape ---------- */
struct Shape;
typedef struct ShapeVtbl
{
	void (*draw)(struct Shape *self, Canvas *c);
	void (*edit)(struct Shape *self);
	void (*destroy)(struct Shape *self);
	const char *(*get_name)(struct Shape *self);
	void (*get_bounds)(struct Shape *self, double *min_x, double *max_x, double *min_y, double *max_y);
	void (*save)(struct Shape *self, FILE *f);
	struct Shape *(*load)(FILE *f);
} ShapeVtbl;

typedef struct Shape
{
	ShapeVtbl *vtbl;
	int x, y;
} Shape;

/* ---------- Класс Line ---------- */
typedef struct
{
	Shape base;
	int x2, y2;
} Line;

void line_draw(Shape *s, Canvas *c);
void line_edit(Shape *s);
void line_destroy(Shape *s);
const char *line_get_name(Shape *s);
void line_get_bounds(Shape *s, double *min_x, double *max_x, double *min_y, double *max_y);
void line_save(Shape *s, FILE *f);
Shape *line_load(FILE *f);
Shape *line_create(int x1, int y1, int x2, int y2);

static void bresenham_line(Canvas *c, int x1, int y1, int x2, int y2, char ch)
{
	int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
	int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
	int err = dx + dy, e2;
	while (1)
	{
		put_pixel(c, x1, y1, ch);
		if (x1 == x2 && y1 == y2)
			break;
		e2 = 2 * err;
		if (e2 >= dy)
		{
			err += dy;
			x1 += sx;
		}
		if (e2 <= dx)
		{
			err += dx;
			y1 += sy;
		}
	}
}

void line_draw(Shape *s, Canvas *c)
{
	Line *l = (Line *)s;
	bresenham_line(c, s->x, s->y, l->x2, l->y2, '*');
}

void line_edit(Shape *s)
{
	Line *l = (Line *)s;
	printf("Редактирование линии (текущие координаты: (%d,%d) -> (%d,%d))\n", s->x, s->y, l->x2, l->y2);
	printf("Введите 4 целых числа: x1 y1 x2 y2 (начало и конец линии)\n");
	scanf("%d %d %d %d", &s->x, &s->y, &l->x2, &l->y2);
}

void line_destroy(Shape *s)
{
	free(s);
}

const char *line_get_name(Shape *s)
{
	return "Линия";
}

void line_get_bounds(Shape *s, double *min_x, double *max_x, double *min_y, double *max_y)
{
	Line *l = (Line *)s;
	*min_x = fmin(s->x, l->x2);
	*max_x = fmax(s->x, l->x2);
	*min_y = fmin(s->y, l->y2);
	*max_y = fmax(s->y, l->y2);
}

void line_save(Shape *s, FILE *f)
{
	Line *l = (Line *)s;
	fprintf(f, "1 %d %d %d %d\n", s->x, s->y, l->x2, l->y2);
}

Shape *line_load(FILE *f)
{
	int x1, y1, x2, y2;
	fscanf(f, "%d %d %d %d", &x1, &y1, &x2, &y2);
	return line_create(x1, y1, x2, y2);
}

ShapeVtbl line_vtbl = {
		.draw = line_draw,
		.edit = line_edit,
		.destroy = line_destroy,
		.get_name = line_get_name,
		.get_bounds = line_get_bounds,
		.save = line_save,
		.load = line_load};

Shape *line_create(int x1, int y1, int x2, int y2)
{
	Line *l = (Line *)malloc(sizeof(Line));
	if (!l)
		return NULL;
	l->base.vtbl = &line_vtbl;
	l->base.x = x1;
	l->base.y = y1;
	l->x2 = x2;
	l->y2 = y2;
	return (Shape *)l;
}

/* ---------- Класс MyRectangle ---------- */
typedef struct
{
	Shape base;
	int width, height;
} MyRectangle;

void myrectangle_draw(Shape *s, Canvas *c);
void myrectangle_edit(Shape *s);
void myrectangle_destroy(Shape *s);
const char *myrectangle_get_name(Shape *s);
void myrectangle_get_bounds(Shape *s, double *min_x, double *max_x, double *min_y, double *max_y);
void myrectangle_save(Shape *s, FILE *f);
Shape *myrectangle_load(FILE *f);
Shape *myrectangle_create(int x, int y, int w, int h);

void myrectangle_draw(Shape *s, Canvas *c)
{
	MyRectangle *r = (MyRectangle *)s;
	int x1 = s->x, y1 = s->y;
	int x2 = s->x + r->width - 1;
	int y2 = s->y + r->height - 1;
	for (int x = x1; x <= x2; x++)
	{
		put_pixel(c, x, y1, '-');
		put_pixel(c, x, y2, '-');
	}
	for (int y = y1; y <= y2; y++)
	{
		put_pixel(c, x1, y, '|');
		put_pixel(c, x2, y, '|');
	}
	put_pixel(c, x1, y1, '+');
	put_pixel(c, x2, y1, '+');
	put_pixel(c, x1, y2, '+');
	put_pixel(c, x2, y2, '+');
}

void myrectangle_edit(Shape *s)
{
	MyRectangle *r = (MyRectangle *)s;
	printf("Редактирование прямоугольника (текущие: левый нижний угол (%d,%d), ширина=%d, высота=%d)\n",
				 s->x, s->y, r->width, r->height);
	printf("Введите 4 целых числа: x y width height (координаты левого нижнего угла, ширина, высота)\n");
	scanf("%d %d %d %d", &s->x, &s->y, &r->width, &r->height);
	if (r->width < 2)
		r->width = 2;
	if (r->height < 2)
		r->height = 2;
}

void myrectangle_destroy(Shape *s)
{
	free(s);
}

const char *myrectangle_get_name(Shape *s)
{
	return "Прямоугольник";
}

void myrectangle_get_bounds(Shape *s, double *min_x, double *max_x, double *min_y, double *max_y)
{
	MyRectangle *r = (MyRectangle *)s;
	*min_x = s->x;
	*max_x = s->x + r->width - 1;
	*min_y = s->y;
	*max_y = s->y + r->height - 1;
}

void myrectangle_save(Shape *s, FILE *f)
{
	MyRectangle *r = (MyRectangle *)s;
	fprintf(f, "2 %d %d %d %d\n", s->x, s->y, r->width, r->height);
}

Shape *myrectangle_load(FILE *f)
{
	int x, y, w, h;
	fscanf(f, "%d %d %d %d", &x, &y, &w, &h);
	return myrectangle_create(x, y, w, h);
}

ShapeVtbl myrectangle_vtbl = {
		.draw = myrectangle_draw,
		.edit = myrectangle_edit,
		.destroy = myrectangle_destroy,
		.get_name = myrectangle_get_name,
		.get_bounds = myrectangle_get_bounds,
		.save = myrectangle_save,
		.load = myrectangle_load};

Shape *myrectangle_create(int x, int y, int w, int h)
{
	if (w < 2)
		w = 2;
	if (h < 2)
		h = 2;
	MyRectangle *r = (MyRectangle *)malloc(sizeof(MyRectangle));
	if (!r)
		return NULL;
	r->base.vtbl = &myrectangle_vtbl;
	r->base.x = x;
	r->base.y = y;
	r->width = w;
	r->height = h;
	return (Shape *)r;
}

/* ---------- Класс Circle ---------- */
typedef struct
{
	Shape base;
	int radius;
} Circle;

void circle_draw(Shape *s, Canvas *c);
void circle_edit(Shape *s);
void circle_destroy(Shape *s);
const char *circle_get_name(Shape *s);
void circle_get_bounds(Shape *s, double *min_x, double *max_x, double *min_y, double *max_y);
void circle_save(Shape *s, FILE *f);
Shape *circle_load(FILE *f);
Shape *circle_create(int x, int y, int r);

static void circle_points(Canvas *c, int cx, int cy, int x, int y, char ch)
{
	put_pixel(c, cx + x, cy + y, ch);
	put_pixel(c, cx - x, cy + y, ch);
	put_pixel(c, cx + x, cy - y, ch);
	put_pixel(c, cx - x, cy - y, ch);
	put_pixel(c, cx + y, cy + x, ch);
	put_pixel(c, cx - y, cy + x, ch);
	put_pixel(c, cx + y, cy - x, ch);
	put_pixel(c, cx - y, cy - x, ch);
}

void circle_draw(Shape *s, Canvas *c)
{
	Circle *circ = (Circle *)s;
	int x = 0, y = circ->radius;
	int d = 3 - 2 * circ->radius;
	circle_points(c, s->x, s->y, x, y, '*');
	while (y >= x)
	{
		x++;
		if (d > 0)
		{
			y--;
			d = d + 4 * (x - y) + 10;
		}
		else
		{
			d = d + 4 * x + 6;
		}
		circle_points(c, s->x, s->y, x, y, '*');
	}
}

void circle_edit(Shape *s)
{
	Circle *circ = (Circle *)s;
	printf("Редактирование окружности (текущие: центр (%d,%d), радиус=%d)\n", s->x, s->y, circ->radius);
	printf("Введите 3 целых числа: x y radius (центр и радиус)\n");
	scanf("%d %d %d", &s->x, &s->y, &circ->radius);
	if (circ->radius < 1)
		circ->radius = 1;
}

void circle_destroy(Shape *s)
{
	free(s);
}

const char *circle_get_name(Shape *s)
{
	return "Окружность";
}

void circle_get_bounds(Shape *s, double *min_x, double *max_x, double *min_y, double *max_y)
{
	Circle *circ = (Circle *)s;
	*min_x = s->x - circ->radius;
	*max_x = s->x + circ->radius;
	*min_y = s->y - circ->radius;
	*max_y = s->y + circ->radius;
}

void circle_save(Shape *s, FILE *f)
{
	Circle *circ = (Circle *)s;
	fprintf(f, "3 %d %d %d\n", s->x, s->y, circ->radius);
}

Shape *circle_load(FILE *f)
{
	int x, y, r;
	fscanf(f, "%d %d %d", &x, &y, &r);
	return circle_create(x, y, r);
}

ShapeVtbl circle_vtbl = {
		.draw = circle_draw,
		.edit = circle_edit,
		.destroy = circle_destroy,
		.get_name = circle_get_name,
		.get_bounds = circle_get_bounds,
		.save = circle_save,
		.load = circle_load};

Shape *circle_create(int x, int y, int r)
{
	if (r < 1)
		r = 1;
	Circle *circ = (Circle *)malloc(sizeof(Circle));
	if (!circ)
		return NULL;
	circ->base.vtbl = &circle_vtbl;
	circ->base.x = x;
	circ->base.y = y;
	circ->radius = r;
	return (Shape *)circ;
}

/* ---------- Класс Triangle (по трём точкам) ---------- */
typedef struct
{
	Shape base;
	int x2, y2;
	int x3, y3;
} Triangle;

void triangle_draw(Shape *s, Canvas *c);
void triangle_edit(Shape *s);
void triangle_destroy(Shape *s);
const char *triangle_get_name(Shape *s);
void triangle_get_bounds(Shape *s, double *min_x, double *max_x, double *min_y, double *max_y);
void triangle_save(Shape *s, FILE *f);
Shape *triangle_load(FILE *f);
Shape *triangle_create(int x1, int y1, int x2, int y2, int x3, int y3);

void triangle_draw(Shape *s, Canvas *c)
{
	Triangle *t = (Triangle *)s;
	bresenham_line(c, s->x, s->y, t->x2, t->y2, '*');
	bresenham_line(c, t->x2, t->y2, t->x3, t->y3, '*');
	bresenham_line(c, t->x3, t->y3, s->x, s->y, '*');
}

void triangle_edit(Shape *s)
{
	Triangle *t = (Triangle *)s;
	printf("Редактирование треугольника (текущие вершины: (%d,%d), (%d,%d), (%d,%d))\n",
				 s->x, s->y, t->x2, t->y2, t->x3, t->y3);
	printf("Введите 6 целых чисел: x1 y1 x2 y2 x3 y3 (координаты вершин)\n");
	scanf("%d %d %d %d %d %d", &s->x, &s->y, &t->x2, &t->y2, &t->x3, &t->y3);
}

void triangle_destroy(Shape *s)
{
	free(s);
}

const char *triangle_get_name(Shape *s)
{
	return "Треугольник";
}

void triangle_get_bounds(Shape *s, double *min_x, double *max_x, double *min_y, double *max_y)
{
	Triangle *t = (Triangle *)s;
	*min_x = fmin(fmin(s->x, t->x2), t->x3);
	*max_x = fmax(fmax(s->x, t->x2), t->x3);
	*min_y = fmin(fmin(s->y, t->y2), t->y3);
	*max_y = fmax(fmax(s->y, t->y2), t->y3);
}

void triangle_save(Shape *s, FILE *f)
{
	Triangle *t = (Triangle *)s;
	fprintf(f, "4 %d %d %d %d %d %d\n", s->x, s->y, t->x2, t->y2, t->x3, t->y3);
}

Shape *triangle_load(FILE *f)
{
	int x1, y1, x2, y2, x3, y3;
	fscanf(f, "%d %d %d %d %d %d", &x1, &y1, &x2, &y2, &x3, &y3);
	return triangle_create(x1, y1, x2, y2, x3, y3);
}

ShapeVtbl triangle_vtbl = {
		.draw = triangle_draw,
		.edit = triangle_edit,
		.destroy = triangle_destroy,
		.get_name = triangle_get_name,
		.get_bounds = triangle_get_bounds,
		.save = triangle_save,
		.load = triangle_load};

Shape *triangle_create(int x1, int y1, int x2, int y2, int x3, int y3)
{
	Triangle *t = (Triangle *)malloc(sizeof(Triangle));
	if (!t)
		return NULL;
	t->base.vtbl = &triangle_vtbl;
	t->base.x = x1;
	t->base.y = y1;
	t->x2 = x2;
	t->y2 = y2;
	t->x3 = x3;
	t->y3 = y3;
	return (Shape *)t;
}

/* ---------- Класс MyEllipse (центр, горизонтальный и вертикальный радиусы) ---------- */
typedef struct
{
	Shape base;
	int rx, ry; // горизонтальный и вертикальный радиусы
} MyEllipse;

void myellipse_draw(Shape *s, Canvas *c);
void myellipse_edit(Shape *s);
void myellipse_destroy(Shape *s);
const char *myellipse_get_name(Shape *s);
void myellipse_get_bounds(Shape *s, double *min_x, double *max_x, double *min_y, double *max_y);
void myellipse_save(Shape *s, FILE *f);
Shape *myellipse_load(FILE *f);
Shape *myellipse_create(int x, int y, int rx, int ry);

static void myellipse_points(Canvas *c, int cx, int cy, int x, int y, char ch)
{
	put_pixel(c, cx + x, cy + y, ch);
	put_pixel(c, cx - x, cy + y, ch);
	put_pixel(c, cx + x, cy - y, ch);
	put_pixel(c, cx - x, cy - y, ch);
}

void myellipse_draw(Shape *s, Canvas *c)
{
	MyEllipse *e = (MyEllipse *)s;
	int cx = s->x, cy = s->y;
	int rx = e->rx, ry = e->ry;
	int x = 0, y = ry;
	int rx2 = rx * rx, ry2 = ry * ry;
	int err = ry2 - rx2 * ry + rx2 / 4;
	myellipse_points(c, cx, cy, x, y, '*');
	while (2 * ry2 * x < 2 * rx2 * y)
	{
		x++;
		if (err < 0)
			err += 2 * ry2 * x + ry2;
		else
		{
			y--;
			err += 2 * ry2 * x + ry2 - 2 * rx2 * y;
		}
		myellipse_points(c, cx, cy, x, y, '*');
	}
	err = ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2;
	while (y >= 0)
	{
		myellipse_points(c, cx, cy, x, y, '*');
		y--;
		if (err > 0)
			err -= 2 * rx2 * y + rx2;
		else
		{
			x++;
			err += 2 * ry2 * x - 2 * rx2 * y + rx2;
		}
	}
}

void myellipse_edit(Shape *s)
{
	MyEllipse *e = (MyEllipse *)s;
	printf("Редактирование эллипса (текущие: центр (%d,%d), горизонтальный радиус=%d, вертикальный радиус=%d)\n",
				 s->x, s->y, e->rx, e->ry);
	printf("Введите 4 целых числа: x y rx ry (центр, горизонтальный радиус, вертикальный радиус)\n");
	scanf("%d %d %d %d", &s->x, &s->y, &e->rx, &e->ry);
	if (e->rx < 1)
		e->rx = 1;
	if (e->ry < 1)
		e->ry = 1;
}

void myellipse_destroy(Shape *s)
{
	free(s);
}

const char *myellipse_get_name(Shape *s)
{
	return "Эллипс";
}

void myellipse_get_bounds(Shape *s, double *min_x, double *max_x, double *min_y, double *max_y)
{
	MyEllipse *e = (MyEllipse *)s;
	*min_x = s->x - e->rx;
	*max_x = s->x + e->rx;
	*min_y = s->y - e->ry;
	*max_y = s->y + e->ry;
}

void myellipse_save(Shape *s, FILE *f)
{
	MyEllipse *e = (MyEllipse *)s;
	fprintf(f, "5 %d %d %d %d\n", s->x, s->y, e->rx, e->ry);
}

Shape *myellipse_load(FILE *f)
{
	int x, y, rx, ry;
	fscanf(f, "%d %d %d %d", &x, &y, &rx, &ry);
	return myellipse_create(x, y, rx, ry);
}

ShapeVtbl myellipse_vtbl = {
		.draw = myellipse_draw,
		.edit = myellipse_edit,
		.destroy = myellipse_destroy,
		.get_name = myellipse_get_name,
		.get_bounds = myellipse_get_bounds,
		.save = myellipse_save,
		.load = myellipse_load};

Shape *myellipse_create(int x, int y, int rx, int ry)
{
	if (rx < 1)
		rx = 1;
	if (ry < 1)
		ry = 1;
	MyEllipse *e = (MyEllipse *)malloc(sizeof(MyEllipse));
	if (!e)
		return NULL;
	e->base.vtbl = &myellipse_vtbl;
	e->base.x = x;
	e->base.y = y;
	e->rx = rx;
	e->ry = ry;
	return (Shape *)e;
}

/* ---------- Управление списком фигур ---------- */
typedef struct
{
	Shape **shapes;
	int count;
	int capacity;
} ShapeList;

ShapeList *list_create(int initial_capacity);
void list_destroy(ShapeList *list);
void list_add(ShapeList *list, Shape *s);
void list_remove(ShapeList *list, int index);
void list_display_all(ShapeList *list, Canvas *c);
void list_edit_shape(ShapeList *list, int index);
void list_recalc_bounds(ShapeList *list, Canvas *c);
void list_save_to_file(ShapeList *list, const char *filename);
void list_load_from_file(ShapeList *list, const char *filename);

ShapeList *list_create(int initial_capacity)
{
	ShapeList *list = (ShapeList *)malloc(sizeof(ShapeList));
	if (!list)
		return NULL;
	list->shapes = (Shape **)malloc(initial_capacity * sizeof(Shape *));
	if (!list->shapes)
	{
		free(list);
		return NULL;
	}
	list->count = 0;
	list->capacity = initial_capacity;
	return list;
}

void list_destroy(ShapeList *list)
{
	for (int i = 0; i < list->count; i++)
		list->shapes[i]->vtbl->destroy(list->shapes[i]);
	free(list->shapes);
	free(list);
}

void list_add(ShapeList *list, Shape *s)
{
	if (list->count == list->capacity)
	{
		list->capacity *= 2;
		list->shapes = (Shape **)realloc(list->shapes, list->capacity * sizeof(Shape *));
	}
	list->shapes[list->count++] = s;
}

void list_remove(ShapeList *list, int index)
{
	if (index < 0 || index >= list->count)
		return;
	list->shapes[index]->vtbl->destroy(list->shapes[index]);
	for (int i = index; i < list->count - 1; i++)
		list->shapes[i] = list->shapes[i + 1];
	list->count--;
}

void list_recalc_bounds(ShapeList *list, Canvas *c)
{
	if (list->count == 0)
	{
		canvas_configure(c, -10, 10, -10, 10);
		return;
	}
	double min_x = DBL_MAX, max_x = -DBL_MAX, min_y = DBL_MAX, max_y = -DBL_MAX;
	for (int i = 0; i < list->count; i++)
	{
		double s_min_x, s_max_x, s_min_y, s_max_y;
		list->shapes[i]->vtbl->get_bounds(list->shapes[i], &s_min_x, &s_max_x, &s_min_y, &s_max_y);
		if (s_min_x < min_x)
			min_x = s_min_x;
		if (s_max_x > max_x)
			max_x = s_max_x;
		if (s_min_y < min_y)
			min_y = s_min_y;
		if (s_max_y > max_y)
			max_y = s_max_y;
	}
	double dx = (max_x - min_x) * 0.1;
	double dy = (max_y - min_y) * 0.1;
	if (dx == 0)
		dx = 1;
	if (dy == 0)
		dy = 1;
	canvas_configure(c, min_x - dx, max_x + dx, min_y - dy, max_y + dy);
}

void list_display_all(ShapeList *list, Canvas *c)
{
	list_recalc_bounds(list, c);
	canvas_clear(c);
	draw_axes(c);
	for (int i = 0; i < list->count; i++)
		list->shapes[i]->vtbl->draw(list->shapes[i], c);
	canvas_display(c);
}

void list_edit_shape(ShapeList *list, int index)
{
	if (index < 0 || index >= list->count)
		return;
	list->shapes[index]->vtbl->edit(list->shapes[index]);
}

void list_save_to_file(ShapeList *list, const char *filename)
{
	FILE *f = fopen(filename, "w");
	if (!f)
	{
		printf("Ошибка: не удалось открыть файл для записи.\n");
		return;
	}
	fprintf(f, "%d\n", list->count);
	for (int i = 0; i < list->count; i++)
		list->shapes[i]->vtbl->save(list->shapes[i], f);
	fclose(f);
	printf("Сохранено %d фигур в файл '%s'.\n", list->count, filename);
}

void list_load_from_file(ShapeList *list, const char *filename)
{
	FILE *f = fopen(filename, "r");
	if (!f)
	{
		printf("Ошибка: не удалось открыть файл для чтения.\n");
		return;
	}
	int count;
	fscanf(f, "%d", &count);
	// Очищаем текущий список
	for (int i = 0; i < list->count; i++)
		list->shapes[i]->vtbl->destroy(list->shapes[i]);
	list->count = 0;
	// Загружаем новые фигуры
	for (int i = 0; i < count; i++)
	{
		int type;
		fscanf(f, "%d", &type);
		Shape *s = NULL;
		switch (type)
		{
		case 1:
			s = line_load(f);
			break;
		case 2:
			s = myrectangle_load(f);
			break;
		case 3:
			s = circle_load(f);
			break;
		case 4:
			s = triangle_load(f);
			break;
		case 5:
			s = myellipse_load(f);
			break;
		default:
			break;
		}
		if (s)
			list_add(list, s);
	}
	fclose(f);
	printf("Загружено %d фигур из файла '%s'.\n", list->count, filename);
}

/* ---------- Главное меню ---------- */
void print_menu()
{
	printf("\n=== ASCII-геометрия ===\n");
	printf("1. Создать фигуру\n");
	printf("2. Редактировать фигуру\n");
	printf("3. Отобразить все фигуры\n");
	printf("4. Удалить фигуру\n");
	printf("5. Сохранить в файл\n");
	printf("6. Загрузить из файла\n");
	printf("7. Выход\n");
	printf("Ваш выбор: ");
}

void print_shapes(ShapeList *list)
{
	printf("\nСписок фигур:\n");
	for (int i = 0; i < list->count; i++)
	{
		printf("%d: %s (опорная точка %d,%d)\n", i + 1,
					 list->shapes[i]->vtbl->get_name(list->shapes[i]),
					 list->shapes[i]->x, list->shapes[i]->y);
	}
}

int main()
{
#ifdef _WIN32
	SetConsoleOutputCP(65001);
	SetConsoleCP(65001);
#endif

	Canvas *canvas = canvas_create(80, 25);
	if (!canvas)
	{
		fprintf(stderr, "Не удалось создать холст\n");
		return 1;
	}
	ShapeList *list = list_create(4);
	if (!list)
	{
		canvas_destroy(canvas);
		fprintf(stderr, "Не удалось создать список фигур\n");
		return 1;
	}

	int choice, idx, type;
	int x1, y1, x2, y2, x3, y3, w, h, r, rx, ry;

	do
	{
		print_menu();
		scanf("%d", &choice);
		switch (choice)
		{
		case 1:
			printf("Тип фигуры: 1-Линия, 2-Прямоугольник, 3-Окружность, 4-Треугольник, 5-Эллипс: ");
			scanf("%d", &type);
			switch (type)
			{
			case 1:
				printf("Введите 4 целых числа: x1 y1 x2 y2 (начало и конец линии)\n");
				scanf("%d %d %d %d", &x1, &y1, &x2, &y2);
				list_add(list, line_create(x1, y1, x2, y2));
				break;
			case 2:
				printf("Введите 4 целых числа: x y width height (левый нижний угол, ширина, высота)\n");
				scanf("%d %d %d %d", &x1, &y1, &w, &h);
				list_add(list, myrectangle_create(x1, y1, w, h));
				break;
			case 3:
				printf("Введите 3 целых числа: x y radius (центр и радиус)\n");
				scanf("%d %d %d", &x1, &y1, &r);
				list_add(list, circle_create(x1, y1, r));
				break;
			case 4:
				printf("Введите 6 целых чисел: x1 y1 x2 y2 x3 y3 (координаты вершин треугольника)\n");
				scanf("%d %d %d %d %d %d", &x1, &y1, &x2, &y2, &x3, &y3);
				list_add(list, triangle_create(x1, y1, x2, y2, x3, y3));
				break;
			case 5:
				printf("Введите 4 целых числа: x y rx ry (центр, горизонтальный радиус, вертикальный радиус)\n");
				scanf("%d %d %d %d", &x1, &y1, &rx, &ry);
				list_add(list, myellipse_create(x1, y1, rx, ry));
				break;
			default:
				printf("Неверный тип\n");
			}
			break;
		case 2:
			if (list->count == 0)
			{
				printf("Нет фигур для редактирования.\n");
				break;
			}
			print_shapes(list);
			printf("Выберите номер фигуры для редактирования: ");
			scanf("%d", &idx);
			if (idx >= 1 && idx <= list->count)
				list_edit_shape(list, idx - 1);
			else
				printf("Неверный индекс\n");
			break;
		case 3:
			if (list->count == 0)
				printf("Нет фигур для отображения.\n");
			else
				list_display_all(list, canvas);
			break;
		case 4:
			if (list->count == 0)
			{
				printf("Нет фигур для удаления.\n");
				break;
			}
			print_shapes(list);
			printf("Выберите номер фигуры для удаления: ");
			scanf("%d", &idx);
			if (idx >= 1 && idx <= list->count)
				list_remove(list, idx - 1);
			else
				printf("Неверный индекс\n");
			break;
		case 5:
			list_save_to_file(list, "figures.txt");
			break;
		case 6:
			list_load_from_file(list, "figures.txt");
			break;
		case 7:
			printf("Выход из программы.\n");
			break;
		default:
			printf("Неверный выбор. Повторите.\n");
		}
	} while (choice != 7);

	list_destroy(list);
	canvas_destroy(canvas);
	return 0;
}