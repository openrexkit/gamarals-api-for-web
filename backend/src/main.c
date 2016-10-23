#include <signal.h>

#include <ulfius.h>

#include "config.h"
#include "common.h"
#include "compile.h"
#include "mcu.h"
#include "project.h"

static void sig_nop(int);
static int default_get(const struct _u_request *, struct _u_response *, void *);

int
main(int argc, char *argv[])
{
	struct _u_instance instance;
	int rc;

	UNUSED(argc);
	UNUSED(argv);

	y_init_logs("credentarius", Y_LOG_MODE_CONSOLE, Y_LOG_LEVEL_DEBUG, NULL, "Starting credentarius");

	rc = ulfius_init_instance(&instance, PORT, NULL);
	if (U_OK != rc) {
		y_log_message(Y_LOG_LEVEL_ERROR, "Failed to initialize ulfius!");
		rc = EXIT_FAILURE;
		goto cleanup_logs;
	}

	u_map_put(instance.default_headers, "Access-Control-Allow-Origin", "*");
	instance.max_post_body_size = 1024 * 1024; /* 1 MB post limit */

	ulfius_add_endpoint_by_val(&instance, "DELETE", PREFIX, "/project/:id/file", NULL, NULL, NULL, &project_delete_file, NULL);
	ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/project", NULL, NULL, NULL, &project_get_list, NULL);
	ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/project/:id", NULL, NULL, NULL, &project_get_files, NULL);
	ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/project/:id/file", NULL, NULL, NULL, &project_get_file, NULL);
	ulfius_add_endpoint_by_val(&instance, "PUT", PREFIX, "/project/:id/file", NULL, NULL, NULL, &project_put_file, NULL);
	ulfius_add_endpoint_by_val(&instance, "PUT", PREFIX, "/project/new", NULL, NULL, NULL, &project_put_new, NULL);

	ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/compile/:id", NULL, NULL, NULL, &compile_get_log, NULL);
	ulfius_add_endpoint_by_val(&instance, "PUT", PREFIX, "/compile/:id", NULL, NULL, NULL, &compile_put_project, NULL);

	ulfius_add_endpoint_by_val(&instance, "PUT", PREFIX, "/mcu/:id", NULL, NULL, NULL, &mcu_put_flash, NULL);
	ulfius_add_endpoint_by_val(&instance, "PUT", PREFIX, "/mcu/reset", NULL, NULL, NULL, &mcu_put_reset, NULL);

	ulfius_set_default_endpoint(&instance, NULL, NULL, NULL, &default_get, NULL);

	rc = ulfius_start_framework(&instance);
	if (U_OK != rc) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Error starting framework");
		rc = EXIT_FAILURE;
		goto cleanup_instance;
	}

	y_log_message(Y_LOG_LEVEL_DEBUG, "Listening on port %d.", instance.port);

	signal(SIGHUP, sig_nop);
	signal(SIGINT, sig_nop);
	signal(SIGQUIT, sig_nop);

	/* wait until we get told to exit */
	pause();

	ulfius_stop_framework(&instance);

	rc = EXIT_SUCCESS;

cleanup_instance:
	ulfius_clean_instance(&instance);

cleanup_logs:
	y_log_message(Y_LOG_LEVEL_DEBUG, "Exited cleanly.");
	y_close_logs();
	return rc;
}

void
sig_nop(int signal)
{
	UNUSED(signal);
}

int
default_get(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	UNUSED(request);
	UNUSED(user_data);
	return ulfius_set_empty_response(response, 404);
}

