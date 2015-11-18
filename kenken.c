#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define CONSTRAINT_TYPES 3

typedef struct cell_s cell_t;
struct cell_s {
	unsigned long row;
	unsigned long column;
	unsigned long block;
	unsigned long value;
};

typedef struct cage_s cage_t;
struct cage_s {
	unsigned long target;
	unsigned long start;
	int (*tiles_f)(cage_t *, unsigned long, cell_t *);
	unsigned long cells_n;
	cell_t *cells;
	cell_t *cells_last;
	cell_t *cell_first;
	unsigned long *values_last;
};

typedef struct node_s node_t;
struct node_s {
	node_t *right;
	node_t *down;
	node_t *left;
	node_t *top;
	unsigned long rows_n;
	cage_t *cage;
	unsigned long *tile;
	node_t *column;
};

unsigned long read_xy(int, int, int *, unsigned long);
int tiles_add(cage_t *, unsigned long, cell_t *);
int tiles_subtract(cage_t *, unsigned long, cell_t *);
int tiles_multiply(cage_t *, unsigned long, cell_t *);
int tiles_divide(cage_t *, unsigned long, cell_t *);
int check_value(cage_t *, cell_t *);
int add_tile(cage_t *);
void set_column_node(node_t *, node_t *);
void set_cell_row_nodes(unsigned long, cage_t *, unsigned long *, cell_t *, unsigned long, unsigned long);
void set_row_node(node_t *, node_t **, cage_t *, unsigned long *, node_t *);
void link_left(node_t *, node_t *);
void link_top(node_t *, node_t *);
unsigned long get_row_offset(unsigned long, unsigned long);
unsigned long get_column_offset(unsigned long, unsigned long);
void dlx_search(void);
void set_blocks(cage_t *, unsigned long *);
void cover_column(node_t *);
void uncover_column(node_t *);
void free_cages(cage_t *);

unsigned long order, blocks_n, *blocks, *blocks_last, values_n, *values, cost, solutions_n;
cage_t *cages;
node_t *nodes, **tops, *header, *row_node;

int main(void) {
int r, c;
unsigned long cages_n, *block, columns_n, *value, *tile;
cell_t *cell;
cage_t *cages_last, *cage;
node_t *node, **top;
	r = scanf("%lu", &order);
	if (r != 1 || order < 1) {
		fprintf(stderr, "Invalid order\n");
		return EXIT_FAILURE;
	}
	blocks_n = order*order;
	blocks = calloc(blocks_n, sizeof(unsigned long));
	if (!blocks) {
		fprintf(stderr, "Cannot allocate memory for blocks\n");
		return EXIT_FAILURE;
	}
	blocks_last = blocks+blocks_n;
	r = scanf("%lu", &cages_n);
	if (r != 1 || cages_n < 1) {
		fprintf(stderr, "Invalid number of cages\n");
		free(blocks);
		return EXIT_FAILURE;
	}
	cages = malloc(sizeof(cage_t)*cages_n);
	if (!cages) {
		fprintf(stderr, "Cannot allocate memory for cages\n");
		free(blocks);
		return EXIT_FAILURE;
	}
	cages_last = cages+cages_n;
	for (cage = cages; cage < cages_last; cage++) {
		r = scanf("%lu", &cage->target);
		if (r != 1) {
			fprintf(stderr, "Invalid cage target\n");
			free_cages(cage);
			free(blocks);
			return EXIT_FAILURE;
		}
		do {
			c = fgetc(stdin);
		}
		while (isspace(c));
		if (feof(stdin)) {
			fprintf(stderr, "End of input reached before reading operation\n");
			free_cages(cage);
			free(blocks);
			return EXIT_FAILURE;
		}
		switch (c) {
		case '=':
		case '+':
			cage->start = 0;
			cage->tiles_f = tiles_add;
			break;
		case '-':
			cage->start = 0;
			cage->tiles_f = tiles_subtract;
			break;
		case '*':
			cage->start = 1;
			cage->tiles_f = tiles_multiply;
			break;
		case '/':
			cage->start = 1;
			cage->tiles_f = tiles_divide;
			break;
		default:
			fprintf(stderr, "Invalid operation\n");
			free_cages(cage);
			free(blocks);
			return EXIT_FAILURE;
		}
		r = scanf("%lu", &cage->cells_n);
		if (r != 1 || (c == '=' && cage->cells_n != 1) || (c != '=' && cage->cells_n < 2)) {
			fprintf(stderr, "Invalid number of cells\n");
			free_cages(cage);
			free(blocks);
			return EXIT_FAILURE;
		}
		cage->cells = malloc(sizeof(cell_t)*cage->cells_n);
		if (!cage->cells) {
			fprintf(stderr, "Cannot allocate memory for cells\n");
			free_cages(cage);
			free(blocks);
			return EXIT_FAILURE;
		}
		cage->cells_last = cage->cells+cage->cells_n-1;
		do {
			c = fgetc(stdin);
		}
		while (isspace(c));
		for (cell = cage->cells; cell <= cage->cells_last; cell++) {
			cell->column = read_xy('@', 'Z', &c, 27UL);
			if (cell->column >= order) {
				fprintf(stderr, "Invalid cell column\n");
				free_cages(cage+1);
				free(blocks);
				return EXIT_FAILURE;
			}
			cell->row = read_xy('0', '9', &c, 10UL);
			if (cell->row >= order) {
				fprintf(stderr, "Invalid cell row\n");
				free_cages(cage+1);
				free(blocks);
				return EXIT_FAILURE;
			}
			while (isspace(c) && c != '\n') {
				c = fgetc(stdin);
			}
			cell->block = cell->row*order+cell->column;
			block = blocks+cell->block;
			if (*block > 0) {
				fprintf(stderr, "Duplicate cell\n");
				free_cages(cage+1);
				free(blocks);
				return EXIT_FAILURE;
			}
			*block = 1;
		}
	}
	for (block = blocks; block < blocks_last && *block > 0; block++);
	if (block < blocks_last) {
		fprintf(stderr, "Missing cell\n");
		free_cages(cages_last);
		free(blocks);
		return EXIT_FAILURE;
	}
	values_n = 0;
	for (cage = cages; cage < cages_last; cage++) {
		cage->cell_first = NULL;
		cage->tiles_f(cage, cage->start, cage->cells);
		cage->values_last = values+values_n;
	}
	columns_n = blocks_n*CONSTRAINT_TYPES;
	nodes = malloc(sizeof(node_t)*(columns_n+1+values_n*CONSTRAINT_TYPES));
	if (!nodes) {
		fprintf(stderr, "Cannot allocate memory for nodes\n");
		free(values);
		free_cages(cages_last);
		free(blocks);
		return EXIT_FAILURE;
	}
	tops = malloc(sizeof(node_t *)*columns_n);
	if (!tops) {
		fprintf(stderr, "Cannot allocate memory for tops\n");
		free(nodes);
		free(values);
		free_cages(cages_last);
		free(blocks);
		return EXIT_FAILURE;
	}
	header = nodes+columns_n;
	set_column_node(nodes, header);
	for (node = nodes, top = tops; node < header; node++, top++) {
		set_column_node(node+1, node);
		*top = node;
	}
	row_node = header+1;
	value = values;
	for (cage = cages; cage < cages_last; cage++) {
		for (tile = value; tile < cage->values_last; tile += cage->cells_n) {
			set_cell_row_nodes(cage->cells_n*CONSTRAINT_TYPES, cage, tile, cage->cells, get_row_offset(cage->cells->row, *value), get_column_offset(cage->cells->column, *value));
			value++;
			for (cell = cage->cells+1; cell <= cage->cells_last; cell++) {
				set_cell_row_nodes(0UL, cage, tile, cell, get_row_offset(cell->row, *value), get_column_offset(cell->column, *value));
				value++;
			}
		}
	}
	for (node = nodes, top = tops; node < header; node++, top++) {
		link_top(node, *top);
	}
	cost = 0;
	solutions_n = 0;
	dlx_search();
	printf("\nCost %lu\nSolutions %lu\n", cost, solutions_n);
	free(tops);
	free(nodes);
	free(values);
	free_cages(cages_last);
	free(blocks);
	return EXIT_SUCCESS;
}

unsigned long read_xy(int c_min, int c_max, int *c, unsigned long base) {
unsigned long xy;
	if (*c < c_min+1 || *c > c_max) {
		return order;
	}
	xy = (unsigned long)(*c-c_min-1);
	*c = fgetc(stdin);
	while (*c >= c_min && *c <= c_max) {
		xy *= base;
		xy += (unsigned long)(*c-c_min);
		*c = fgetc(stdin);
	}
	return xy;
}

int tiles_add(cage_t *cage, unsigned long total, cell_t *cell) {
	if (cell < cage->cells_last) {
		for (cell->value = 1; cell->value <= order && total+cell->value < cage->target; cell->value++) {
			if (check_value(cage, cell)) {
				if (!tiles_add(cage, total+cell->value, cell+1)) {
					return 0;
				}
			}
		}
	}
	else {
		cell->value = cage->target-total;
		if (cell->value <= order && check_value(cage, cell)) {
			if (!add_tile(cage)) {
				return 0;
			}
		}
	}
	return 1;
}

int tiles_subtract(cage_t *cage, unsigned long total, cell_t *cell) {
unsigned long sum = cage->target+total;
	if (cell < cage->cells_last) {
		if (cage->cell_first) {
			for (cell->value = 1; cell->value < order && sum+cell->value < cage->cell_first->value; cell->value++) {
				if (check_value(cage, cell)) {
					if (!tiles_subtract(cage, total+cell->value, cell+1)) {
						return 0;
					}
				}
			}
		}
		else {
			for (cell->value = 1; cell->value < order && sum+cell->value <= order; cell->value++) {
				if (check_value(cage, cell)) {
					if (!tiles_subtract(cage, total+cell->value, cell+1)) {
						return 0;
					}
				}
			}
			cage->cell_first = cell;
			for (cell->value = order; cell->value > 0 && cell->value > sum; cell->value--) {
				if (check_value(cage, cell)) {
					if (!tiles_subtract(cage, total, cell+1)) {
						return 0;
					}
				}
			}
			cage->cell_first = NULL;
		}
	}
	else {
		cell->value = cage->cell_first ? cage->cell_first->value-sum:sum;
		if (check_value(cage, cell)) {
			if (!add_tile(cage)) {
				return 0;
			}
		}
	}
	return 1;
}

int tiles_multiply(cage_t *cage, unsigned long total, cell_t *cell) {
	if (cell < cage->cells_last) {
		for (cell->value = 1; cell->value <= order && total*cell->value <= cage->target; cell->value++) {
			if (check_value(cage, cell)) {
				if (!tiles_multiply(cage, total*cell->value, cell+1)) {
					return 0;
				}
			}
		}
	}
	else {
		if (cage->target%total == 0) {
			cell->value = cage->target/total;
			if (cell->value <= order && check_value(cage, cell)) {
				if (!add_tile(cage)) {
					return 0;
				}
			}
		}
	}
	return 1;
}

int tiles_divide(cage_t *cage, unsigned long total, cell_t *cell) {
unsigned long product = cage->target*total;
	if (cell < cage->cells_last) {
		if (cage->cell_first) {
			for (cell->value = 1; cell->value <= order && product*cell->value <= cage->cell_first->value; cell->value++) {
				if (cage->cell_first->value%(product*cell->value) == 0 && check_value(cage, cell)) {
					if (!tiles_divide(cage, total*cell->value, cell+1)) {
						return 0;
					}
				}
			}
		}
		else {
			for (cell->value = 1; cell->value <= order && product*cell->value <= order; cell->value++) {
				if (check_value(cage, cell)) {
					if (!tiles_divide(cage, total*cell->value, cell+1)) {
						return 0;
					}
				}
			}
			cage->cell_first = cell;
			for (cell->value = order; cell->value > 0 && cell->value >= product; cell->value--) {
				if (cell->value%product == 0 && check_value(cage, cell)) {
					if (!tiles_divide(cage, total, cell+1)) {
						return 0;
					}
				}
			}
			cage->cell_first = NULL;
		}
	}
	else {
		if (cage->cell_first) {
			cell->value = cage->cell_first->value/product;
			if (cage->cell_first->value%(product*cell->value) > 0) {
				return 1;
			}
		}
		else {
			cell->value = product;
		}
		if (cell->value <= order && check_value(cage, cell)) {
			if (!add_tile(cage)) {
				return 0;
			}
		}
	}
	return 1;
}

int check_value(cage_t *cage, cell_t *checked) {
cell_t *cell;
	for (cell = cage->cells; cell < checked && (cell->value != checked->value || (cell->row != checked->row && cell->column != checked->column)); cell++);
	return cell == checked;
}

int add_tile(cage_t *cage) {
unsigned long *values_tmp, *value;
cell_t *cell;
	if (values_n > 0) {
		values_tmp = realloc(values, sizeof(unsigned long)*(values_n+cage->cells_n));
		if (!values_tmp) {
			fprintf(stderr, "Cannot reallocate memory for values\n");
			free(values);
			return 0;
		}
		values = values_tmp;
	}
	else {
		values = malloc(sizeof(unsigned long)*cage->cells_n);
		if (!values) {
			fprintf(stderr, "Cannot allocate memory for values\n");
			return 0;
		}
	}
	for (cell = cage->cells, value = values+values_n; cell <= cage->cells_last; cell++, value++) {
		*value = cell->value;
	}
	values_n += cage->cells_n;
	return 1;
}

void set_column_node(node_t *node, node_t *left) {
	link_left(node, left);
	node->rows_n = 0;
}

void set_cell_row_nodes(unsigned long left_offset, cage_t *cage, unsigned long *tile, cell_t *cell, unsigned long row_offset, unsigned long column_offset) {
	set_row_node(row_node+left_offset-1, tops+cell->block, cage, tile, nodes+cell->block);
	set_row_node(row_node-1, tops+row_offset, cage, tile, nodes+row_offset);
	set_row_node(row_node-1, tops+column_offset, cage, tile, nodes+column_offset);
}

void set_row_node(node_t *left, node_t **top, cage_t *cage, unsigned long *tile, node_t *column) {
	link_left(row_node, left);
	link_top(row_node, *top);
	row_node->cage = cage;
	row_node->tile = tile;
	row_node->column = column;
	*top = row_node++;
	column->rows_n++;
}

void link_left(node_t *node, node_t *left) {
	node->left = left;
	left->right = node;
}

void link_top(node_t *node, node_t *top) {
	node->top = top;
	top->down = node;
}

unsigned long get_row_offset(unsigned long row, unsigned long value) {
	return blocks_n+row*order+value-1;
}

unsigned long get_column_offset(unsigned long column, unsigned long value) {
	return blocks_n*2+column*order+value-1;
}

void dlx_search(void) {
unsigned long *line, *line_last, *block;
node_t *column_min, *column, *row, *node;
	cost++;
	if (header->right == header) {
		puts("");
		for (line = blocks; line < blocks_last; line += order) {
			line_last = line+order;
			printf("%lu", *line);
			for (block = line+1; block < line_last; block++) {
				printf(" %lu", *block);
			}
			puts("");
		}
		solutions_n++;
	}
	else {
		column_min = header->right;
		for (column = column_min->right; column != header; column = column->right) {
			if (column->rows_n < column_min->rows_n) {
				column_min = column;
			}
		}
		cover_column(column_min);
		for (row = column_min->down; row != column_min; row = row->down) {
			set_blocks(row->cage, row->tile);
			for (node = row->right; node != row; node = node->right) {
				cover_column(node->column);
			}
			dlx_search();
			for (node = row->left; node != row; node = node->left) {
				uncover_column(node->column);
			}
		}
		uncover_column(column_min);
	}
}

void set_blocks(cage_t *cage, unsigned long *tile) {
unsigned long *value;
cell_t *cell;
	for (cell = cage->cells, value = tile; cell <= cage->cells_last; cell++, value++) {
		*(blocks+cell->block) = *value;
	}
}

void cover_column(node_t *column) {
node_t *row, *node;
	column->right->left = column->left;
	column->left->right = column->right;
	for (row = column->down; row != column; row = row->down) {
		for (node = row->right; node != row; node = node->right) {
			node->column->rows_n--;
			node->down->top = node->top;
			node->top->down = node->down;
		}
	}
}

void uncover_column(node_t *column) {
node_t *row, *node;
	for (row = column->top; row != column; row = row->top) {
		for (node = row->left; node != row; node = node->left) {
			node->top->down = node->down->top = node;
			node->column->rows_n++;
		}
	}
	column->left->right = column->right->left = column;
}

void free_cages(cage_t *last) {
cage_t *cage;
	for (cage = cages; cage < last; cage++) {
		free(cage->cells);
	}
	free(cages);
}
