#include "project.h"

#include <jansson.h>
#include <ulfius.h>

#include "common.h"
#include "config.h"

int
project_delete_file(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	UNUSED(request);
	UNUSED(response);
	UNUSED(user_data);

	return U_ERROR;
}

int
project_get_file(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	UNUSED(request);
	UNUSED(response);
	UNUSED(user_data);

	return U_ERROR;
}

int
project_get_files(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	json_t *root;
	const char *id;

	UNUSED(request);
	UNUSED(user_data);

	if (!(id = u_map_get(request->map_url, "id")))
		return U_ERROR_NOT_FOUND;

	if (!(root = json_array()))
		return U_ERROR_MEMORY;

	y_log_message(Y_LOG_LEVEL_DEBUG, "Project files for '%s' requested.", id);

	json_array_append_new(root, json_string("extra1.c"));
	json_array_append_new(root, json_string("extra1.h"));
	json_array_append_new(root, json_string("main.c"));

	return ulfius_set_json_response(response, 200, root);
}

int
project_get_list(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	json_t *root;

	UNUSED(request);
	UNUSED(user_data);

	if (!(root = json_array()))
		return U_ERROR_MEMORY;

	/* TODO: read from filesystem */
	json_array_append_new(root, json_string("project1"));
	json_array_append_new(root, json_string("project2"));
	json_array_append_new(root, json_string("project3"));

	return ulfius_set_json_response(response, 200, root);
}

int
project_put_file(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	UNUSED(request);
	UNUSED(response);
	UNUSED(user_data);

	return U_ERROR;
}

int
project_put_new(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	UNUSED(request);
	UNUSED(response);
	UNUSED(user_data);

	return U_ERROR;
}

