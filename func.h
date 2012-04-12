/**
 * func.h
 * Copyright (C) 2012 Jan Viktorin
 */

#ifndef FUNC_H
#define FUNC_H

/**
 * Functions available for each CGP computation unit.
 */
enum func_t;

/**
 * String representation of the function.
 */
const char *func_to_str(enum func_t f);

/**
 * Returns maximal number of inputs from all functions.
 */
size_t func_inputs_max(void);

/**
 * Returns maximal number of outputs from all functions.
 */
size_t func_outputs_max(void);

#endif
