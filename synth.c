#include "common.h"
#include "function.h"
#include "player.h"
#include "track.h"
#include "xface.h"

#include <string.h>

#include <signal.h>
#include <execinfo.h>

bool running;

void signal_sigsegv() {
#define BT_SIZE 100
	int j, nptrs;
    void *buffer[BT_SIZE];
	char **strings;

	nptrs = backtrace(buffer, BT_SIZE);
	strings = backtrace_symbols(buffer, nptrs);

	if (strings == NULL)
		perror("backtrace_symbols");

	for (j = 0; j < nptrs; j++)
		LOGF("%s", strings[j]);

	free(strings);
#undef BT_SIZE
}

int main(int argc, char **argv) {
	signal(SIGSEGV, signal_sigsegv);

	running = true;
	function_init();
	tracker_init();
	
	if (argc > 1)
		player_init(strdup(argv[1]));
	else
		player_init(NULL);
	
	xface_init();

	/* This parser sucks, but works. */
	char *t = token(), *t1;
	bool parser_ok;
	enum {
		PC_EXIT,
		PC_MAIN,
		PC_TRACK,
		PC_FUNCTION
	} parser_context = PC_MAIN;
	track_t *tmptrack = NULL;
	do {
//		if (t[0] == '#') continue;
		parser_ok = false;

		switch (parser_context) {
			case PC_MAIN: ;;
				if (STREQ(t, "function")) {
					parser_context = PC_FUNCTION;
					parser_ok = true;
				}
				else if (STREQ(t, "track")) {
					parser_context = PC_TRACK;
					tmptrack = track_new(T_FUNCTIONAL); // TODO
					track_set_source(tmptrack, S_REALTIME);
					parser_ok = true;
				}
				else if (STREQ(t, "body")) {
					parser_context = PC_EXIT;
					parser_ok = false;
				}
				break;
			case PC_TRACK: ;;
				if (STREQ(t, "volume")) {
					t = token();
					track_set_volume(tmptrack, atof(t)/100.0);
					parser_ok = true;
				}
				else if (STREQ(t, "function")) {
					if (! tmptrack->param.p_functional.func) {
						t = token();
						track_set_function(tmptrack, get_function_by_name(t));
						parser_ok = true;
					}
					else {
						parser_ok = false;
						parser_context = PC_MAIN;
					}
				}
				else if (STREQ(t, "attack")) {
					t = token();
					t1 = token();
					track_set_attack(tmptrack, get_function_by_name(t), atof(t1)*RATE);
					free(t1);
					parser_ok = true;
				}
				else if (STREQ(t, "release")) {
					t = token();
					t1 = token();
					track_set_release(tmptrack, get_function_by_name(t), atof(t1)*RATE);
					free(t1);
					parser_ok = true;
				}
				else {
					parser_ok = false;
					parser_context = PC_MAIN;
				}
				break;
			case PC_FUNCTION: ;;
				LOGF("found function %s", t);
				jit_function_t tmpfunc = parse_function();
				if (tmpfunc)
					add_function(strdup(t), tmpfunc);
				else
					LOGF("function '%s' parse failed", t);
				parser_context = PC_MAIN;
				parser_ok = true;
				break;
			case PC_EXIT: ;;
				break;
		}

		free(t);
		if (parser_context == PC_EXIT)
			break;
		if (parser_ok)
			t = token();
		if (!t)
			break;
	} while (parser_ok);

	while (1) pause();
	return 0;
}
