/**
 * fitness_gates.c
 * Copyright (C) 2012 Jan Viktorin
 */

#include "fitness.h"
#include "chromo_def.h"

int fitness_compute(const struct chromo_t *c, fitness_t *value)
{
	*value = 0;
	return 0;
}

int fitness_isbest(fitness_t f)
{
	return f == 0;
}