/* $OpenBSD$ */

/*
 * Copyright (c) 2025 Swarm Code Contributors
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "tmux.h"

/*
 * Simple fuzzy search matching for keybind filtering.
 * Pattern is case-insensitive and can match scattered characters.
 */

/* Check if pattern matches text (case-insensitive) */
int
keys_search_match(const char *pattern, const char *text)
{
	const char *p, *t;
	char pc, tc;

	if (pattern == NULL || *pattern == '\0')
		return (1);
	if (text == NULL || *text == '\0')
		return (0);

	p = pattern;
	t = text;

	while (*p != '\0') {
		/* Convert to lowercase for case-insensitive matching */
		pc = (char)tolower((u_char)*p);

		/* Find next matching character in text */
		while (*t != '\0') {
			tc = (char)tolower((u_char)*t);
			if (tc == pc) {
				t++;
				break;
			}
			t++;
		}

		/* If we didn't find the character, no match */
		if (*t == '\0' && tolower((u_char)*t) != pc)
			return (0);

		p++;
	}

	return (1);
}

/* Score a match (higher is better) */
int
keys_search_score(const char *pattern, const char *text)
{
	const char *p, *t, *match_start;
	char pc, tc;
	int score = 0;
	int consecutive = 0;
	int position_bonus = 0;

	if (pattern == NULL || *pattern == '\0')
		return (100);
	if (text == NULL || *text == '\0')
		return (0);

	/* Quick check for exact match */
	if (strcasecmp(pattern, text) == 0)
		return (200);

	/* Check for substring match */
	if (strcasestr(text, pattern) != NULL) {
		score += 100;

		/* Bonus if starts with pattern */
		if (strncasecmp(text, pattern, strlen(pattern)) == 0)
			score += 50;

		return (score);
	}

	/* Fuzzy match scoring */
	p = pattern;
	t = text;
	match_start = NULL;
	position_bonus = 100;

	while (*p != '\0' && *t != '\0') {
		pc = (char)tolower((u_char)*p);
		tc = (char)tolower((u_char)*t);

		if (pc == tc) {
			if (match_start == NULL)
				match_start = t;

			/* Consecutive character bonus */
			consecutive++;
			if (consecutive > 1)
				score += consecutive * 2;
			else
				score += 10;

			/* Bonus for matching early in text */
			score += position_bonus;
			position_bonus = (position_bonus > 0) ? position_bonus - 5 : 0;

			/* Bonus for matching after word boundary */
			if (t > text && (*(t-1) == ' ' || *(t-1) == '-' || *(t-1) == '_'))
				score += 15;

			p++;
		} else {
			consecutive = 0;
		}
		t++;
	}

	/* Penalty if not all pattern characters were matched */
	if (*p != '\0')
		score = 0;

	return (score);
}
