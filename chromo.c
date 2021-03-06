/**
 * chromo.c
 * Copyright (C) 2012 Jan Viktorin
 */

#include "chromo.h"
#include "chromo_def.h"
#include "func.h"
#include "cgp_config.h"
#include "rndgen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

struct chromo_t *chromo_at(struct chromo_t *array, size_t i)
{
	return array + i;
}

static
struct chromo_t *impl_chromos_alloc(size_t count)
{
	return (struct chromo_t *) calloc(count, sizeof(struct chromo_t));
}

static
struct cell_t *impl_cells_alloc(size_t count)
{
	const size_t cells = CGP_WIDTH * CGP_HEIGHT;
	return (struct cell_t *) calloc(count * cells, sizeof(struct cell_t));
}

static
port_t *impl_inputs_alloc(size_t count)
{
	const size_t cells = CGP_WIDTH * CGP_HEIGHT;
	const size_t inputs = cells * func_inputs_max();
	return (port_t *) calloc(count * inputs, sizeof(port_t));
}

static
port_t *impl_outputs_alloc(size_t count)
{
	return (port_t *) calloc(count * CGP_OUTPUTS, sizeof(port_t));
}

struct chromo_t *chromo_alloc(size_t count)
{
	const size_t cells = CGP_WIDTH * CGP_HEIGHT;
	const size_t inputs = cells * func_inputs_max();

	if(count == 0)
		return NULL;

	struct chromo_t *c = impl_chromos_alloc(count);
	struct cell_t *all_cells = impl_cells_alloc(count);
	port_t *all_inputs = impl_inputs_alloc(count);
	port_t *all_outputs = impl_outputs_alloc(count);

	for(size_t i = 0; i < count; ++i) {
		c[i].cell = all_cells + (i * cells);
		c[i].outputs = all_outputs + (i * CGP_OUTPUTS);

		for(size_t j = 0; j < cells; ++j) {
			c[i].cell[j].inputs = all_inputs + (i * inputs) + (j * func_inputs_max());
			c[i].cell[j].id = j;
		}
	}

	return c;
}

void chromo_free(struct chromo_t *c)
{
	if(c != NULL) {
		free(c->cell->inputs);
		free(c->cell);
		free(c->outputs);
		free(c);
	}
}

void chromo_copy(struct chromo_t *dst, const struct chromo_t *src)
{
	if(src == dst)
		return;

	memcpy(dst->outputs, src->outputs, CGP_OUTPUTS * sizeof(port_t));

	for(size_t i = 0; i < CGP_WIDTH * CGP_HEIGHT; ++i) {
		dst->cell[i].next = NULL;
		dst->cell[i].f = src->cell[i].f;
		dst->cell[i].id = src->cell[i].id;

		memcpy(dst->cell[i].inputs, src->cell[i].inputs,
				func_inputs_max() * sizeof(port_t));
	}
}

static
port_t first_port_possible(size_t col)
{
	port_t first_possible = 0;

	if(col == 0)
		first_possible = 0;
	else if(CGP_LBACK > col)
		first_possible = 0;
	else
		first_possible = CGP_INPUTS
			+ (col - CGP_LBACK) * CGP_HEIGHT * func_outputs_max();

	return first_possible;
}

static
port_t last_port_possible(size_t col)
{
	return CGP_INPUTS + col * CGP_HEIGHT * func_outputs_max() - 1;
}

static
port_t port_gen(size_t col)
{
	const size_t first_possible = col >= CGP_WIDTH? 0 : first_port_possible(col);
	const size_t last_possible  = last_port_possible(col);

	assert(last_possible > first_possible);

	const size_t max = last_possible - first_possible;
	return first_possible + rndgen_range(max);
}

void cell_outputs(const struct cell_t *cell, port_t *first, port_t *last)
{
	*first = CGP_INPUTS + cell->id * func_outputs_max();
	*last  = CGP_INPUTS + (cell->id + 1) * func_outputs_max() - 1;

	assert(*first <= *last);
	assert(*last - *first + 1 == func_outputs_max());
}

void chromo_gen(struct chromo_t *c)
{
	assert(CGP_LBACK > 0);
	const size_t cells = CGP_WIDTH * CGP_HEIGHT;

	for(size_t i = 0; i < cells; ++i) {
		func_gen(&c->cell[i].f);
		const size_t col = i / CGP_HEIGHT;

		for(size_t j = 0; j < func_inputs_max(); ++j)
			c->cell[i].inputs[j] = port_gen(col);
	}

	for(size_t i = 0; i < CGP_OUTPUTS; ++i)
		c->outputs[i] = port_gen(CGP_WIDTH);
}

static
void port_mut(port_t *ports, size_t col, size_t i)
{
	ports[i] = port_gen(col);
	assert(ports[i] < CGP_INPUTS + col * CGP_HEIGHT * func_outputs_max());
}

static
void cell_mut(struct cell_t *cells, size_t i, size_t what)
{
	assert(i < CGP_WIDTH * CGP_HEIGHT);
	assert(func_count() <= 1 || what < 1 + func_inputs_max());
	assert(func_count() >  1 || what < func_inputs_max());

	// do not mutate function if it doesn't make sense
	// => there is only one function available
	if(what == 0 && func_count() > 1) {
		func_mut(&cells[i].f);
	}
	else if(func_count() <= 1) {
		const size_t col = i / CGP_HEIGHT;
		port_mut(cells[i].inputs, col, what);
	}
	else {
		const size_t col = i / CGP_HEIGHT;
		port_mut(cells[i].inputs, col, what);
	}
}

static
int run_mut(void)
{
	if(CGP_MUT_PROBABILITY >= 100)
		return 1;

	if(CGP_MUT_PROBABILITY <= 0)
		return 0;

	size_t p = rndgen_range(100);
	return p < CGP_MUT_PROBABILITY;
}

static
size_t count_all_inputs(void)
{
	const size_t cells = CGP_WIDTH * CGP_HEIGHT;
	return CGP_OUTPUTS + cells * (1 + func_inputs_max());
}

static
size_t cell_index_from_input(size_t i)
{
	const size_t items_in_cell = 1 + func_inputs_max();
	assert(items_in_cell > 0);
	return i / items_in_cell;
}

static
size_t index_in_cell(size_t i)
{
	// if there is only one function available, do not mutate it...
	const size_t items_in_cell = func_inputs_max() + (func_count() > 1? 1 : 0);
	assert(items_in_cell > 0);
	return i % items_in_cell;
}

void chromo_mut(struct chromo_t *c)
{
	const size_t inputs = count_all_inputs();

	for(size_t j = 0; j < CGP_MUTS; ++j) {
		if(!run_mut())
			continue;

		size_t i = rndgen_range(inputs - 1);

		if(i < CGP_OUTPUTS) {
			port_mut(c->outputs, CGP_WIDTH, i);
		}
		else {
			const size_t celli = cell_index_from_input(i - CGP_OUTPUTS);
			const size_t what  = index_in_cell(i - CGP_OUTPUTS);
			cell_mut(c->cell, celli, what);
		}
	}
}

void chromo_print(FILE *fout, const struct chromo_t *c)
{
	fprintf(fout, "%d %d ", CGP_WIDTH, CGP_HEIGHT);
	fprintf(fout, "%d %d ", CGP_INPUTS, CGP_OUTPUTS);
	fprintf(fout, "%zu %zu ", func_inputs_max(), func_outputs_max());

	for(size_t i = 0; i < CGP_WIDTH * CGP_HEIGHT; ++i) {
		const struct cell_t *cell = c->cell + i;
		fprintf(fout, "%d ", cell->f);

		for(size_t j = 0; j < func_inputs_max(); ++j)
			fprintf(fout, "%zu ", cell->inputs[j]);
	}

	for(size_t j = 0; j < CGP_OUTPUTS; ++j)
		fprintf(fout, "%zu ", c->outputs[j]);
}

int chromo_parse(FILE *fin, struct chromo_t *c)
{
	size_t cgp_width;
	size_t cgp_height;
	size_t cgp_inputs;
	size_t cgp_outputs;
	size_t inputs_max;
	size_t outputs_max;

	// check chromosome compatibility
	if(fscanf(fin, "%zu %zu", &cgp_width, &cgp_height) != 2)
		return 1;
	if(cgp_width != CGP_WIDTH || cgp_height != CGP_HEIGHT)
		return 2;
	if(fscanf(fin, "%zu %zu", &cgp_inputs, &cgp_outputs) != 2)
		return 3;
	if(cgp_inputs != CGP_INPUTS || cgp_outputs != CGP_OUTPUTS)
		return 4;
	if(fscanf(fin, "%zu %zu", &inputs_max, &outputs_max) != 2)
		return 5;
	if(inputs_max != func_inputs_max() || outputs_max != func_outputs_max())
		return 6;

	for(size_t i = 0; i < CGP_WIDTH * CGP_HEIGHT; ++i) {
		struct cell_t *cell = c->cell + i;
		if(fscanf(fin, FUNC_FMT, &cell->f) != 1)
			return 7;

		for(size_t j = 0; j < func_inputs_max(); ++j) {
			if(fscanf(fin, "%zu", &cell->inputs[j]) != 1)
				return 8;
		}
	}

	for(size_t j = 0; j < CGP_OUTPUTS; ++j) {
		if(fscanf(fin, "%zu", &c->outputs[j]) != 1)
			return 9;
	}

	return 0;
}
