#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <windows.h>

/* ---------- Холст (Canvas) ---------- */
typedef struct Canvas
{
	int width;
	int height;
	char *buffer;
} Canvas;

// Прототипы функций холста
Canvas *canvas_create(int width, int height);
void canvas_destroy(Canvas *c);
void canvas_clear(Canvas *c);
void canvas_put_char(Canvas *c, int x, int y, char ch);
void canvas_display(Canvas *c);

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

/* ---------- Базовый класс Shape (виртуальная таблица) ---------- */
struct Shape;

typedef struct ShapeVtbl
{
	void (*draw)(struct Shape *self, Canvas *c);
	void (*edit)(struct Shape *self);
	void (*destroy)(struct Shape *self);
	const char *(*get_name)(struct Shape *self);
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

// Прототипы для Line
void line_draw(Shape *s, Canvas *c);
void line_edit(Shape *s);
void line_destroy(Shape *s);
const char *line_get_name(Shape *s);
Shape *line_create(int x1, int y1, int x2, int y2);

static void bresenham_line(Canvas *c, int x1, int y1, int x2, int y2, char ch)
{
	int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
	int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
	int err = dx + dy, e2;
	while (1)
	{
		canvas_put_char(c, x1, y1, ch);
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
	printf("Введите новый x1, y1, x2, y2: ");
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

ShapeVtbl line_vtbl = {
		.draw = line_draw,
		.edit = line_edit,
		.destroy = line_destroy,
		.get_name = line_get_name};

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

/* ---------- Класс MyRectangle (новое имя, чтобы не конфликтовать с Windows API) ---------- */
typedef struct
{
	Shape base;
	int width, height;
} MyRectangle;

void myrectangle_draw(Shape *s, Canvas *c);
void myrectangle_edit(Shape *s);
void myrectangle_destroy(Shape *s);
const char *myrectangle_get_name(Shape *s);
Shape *myrectangle_create(int x, int y, int w, int h);

void myrectangle_draw(Shape *s, Canvas *c)
{
	MyRectangle *r = (MyRectangle *)s;
	int x1 = s->x, y1 = s->y;
	int x2 = s->x + r->width - 1;
	int y2 = s->y + r->height - 1;
	for (int x = x1; x <= x2; x++)
	{
		canvas_put_char(c, x, y1, '-');
		canvas_put_char(c, x, y2, '-');
	}
	for (int y = y1; y <= y2; y++)
	{
		canvas_put_char(c, x1, y, '|');
		canvas_put_char(c, x2, y, '|');
	}
	canvas_put_char(c, x1, y1, '+');
	canvas_put_char(c, x2, y1, '+');
	canvas_put_char(c, x1, y2, '+');
	canvas_put_char(c, x2, y2, '+');
}

void myrectangle_edit(Shape *s)
{
	MyRectangle *r = (MyRectangle *)s;
	printf("Редактирование прямоугольника (текущие: левый верхний (%d,%d), ширина=%d, высота=%d)\n",
				 s->x, s->y, r->width, r->height);
	printf("Введите новые x, y, ширину, высоту: ");
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

ShapeVtbl myrectangle_vtbl = {
		.draw = myrectangle_draw,
		.edit = myrectangle_edit,
		.destroy = myrectangle_destroy,
		.get_name = myrectangle_get_name};

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
Shape *circle_create(int x, int y, int r);

static void circle_points(Canvas *c, int cx, int cy, int x, int y, char ch)
{
	canvas_put_char(c, cx + x, cy + y, ch);
	canvas_put_char(c, cx - x, cy + y, ch);
	canvas_put_char(c, cx + x, cy - y, ch);
	canvas_put_char(c, cx - x, cy - y, ch);
	canvas_put_char(c, cx + y, cy + x, ch);
	canvas_put_char(c, cx - y, cy + x, ch);
	canvas_put_char(c, cx + y, cy - x, ch);
	canvas_put_char(c, cx - y, cy - x, ch);
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
	printf("Введите новые x, y (центр) и радиус: ");
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

ShapeVtbl circle_vtbl = {
		.draw = circle_draw,
		.edit = circle_edit,
		.destroy = circle_destroy,
		.get_name = circle_get_name};

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

void list_display_all(ShapeList *list, Canvas *c)
{
	canvas_clear(c);
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

/* ---------- Главное меню ---------- */
void print_menu()
{
	printf("\n=== ASCII-геометрия ===\n");
	printf("1. Создать фигуру\n");
	printf("2. Редактировать фигуру\n");
	printf("3. Отобразить все фигуры\n");
	printf("4. Удалить фигуру\n");
	printf("5. Выход\n");
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
	int x1, y1, x2, y2, w, h, r;

	do
	{
		print_menu();
		scanf("%d", &choice);
		switch (choice)
		{
		case 1:
			printf("Тип фигуры: 1-Линия, 2-Прямоугольник, 3-Окружность: ");
			scanf("%d", &type);
			switch (type)
			{
			case 1:
				printf("Введите x1 y1 x2 y2: ");
				scanf("%d %d %d %d", &x1, &y1, &x2, &y2);
				list_add(list, line_create(x1, y1, x2, y2));
				break;
			case 2:
				printf("Введите x y ширину высоту: ");
				scanf("%d %d %d %d", &x1, &y1, &w, &h);
				list_add(list, myrectangle_create(x1, y1, w, h));
				break;
			case 3:
				printf("Введите x y радиус: ");
				scanf("%d %d %d", &x1, &y1, &r);
				list_add(list, circle_create(x1, y1, r));
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
			printf("Выход из программы.\n");
			break;
		default:
			printf("Неверный выбор. Повторите.\n");
		}
	} while (choice != 5);

	list_destroy(list);
	canvas_destroy(canvas);
	return 0;
}