#include "project.h"

#include <sys/stat.h>

#include <dirent.h>
#include <fcntl.h>
#include <jansson.h>
#include <ulfius.h>
#include <unistd.h>

#include "common.h"
#include "config.h"

static int _project_path_check(const char *, int);

int
project_delete_existing(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	char path[PATH_MAX] = {0};
	struct dirent *dir;
	const char *id;
	DIR *d;
	int rc;

	UNUSED(user_data);

	if (!(id = u_map_get(request->map_url, "id"))) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No project id was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	rc = snprintf(path, sizeof(path), "%s/%s", PROJECT_PATH, id);
	if (rc <= 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to generate project path.");
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	if (_project_path_check(path, FALSE) != 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Tried to delete project that doesn't exist: %s", id);
		return ulfius_set_empty_response(response, HTTP_NOT_FOUND);
	}

	if (!(d = opendir(path))) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to iterate through directory: %s", path);
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	/* currently not supporting multiple directory levels */

	while ((dir = readdir(d))) {
		switch (dir->d_type) {
		case DT_REG:
		case DT_LNK:
			if (unlinkat(dirfd(d), dir->d_name, 0) == -1) {
				y_log_message(Y_LOG_LEVEL_DEBUG,
				    "Failed to delete file in project '%s': %s",
				    id, dir->d_name);
			}
			break;
		case DT_DIR:
			if (strcmp(dir->d_name, ".") == 0 ||
			    strcmp(dir->d_name, "..") == 0) {
				break;
			}

			/* fall-through */
		default:
			y_log_message(Y_LOG_LEVEL_DEBUG,
			    "Unsupported file uncounted in project '%s': %s",
			    id, dir->d_name);
			break;
		}
	}

	closedir(d);

	if (rmdir(path) == -1) {
		y_log_message(Y_LOG_LEVEL_ERROR,
		    "Failed to remove project directory: %s", path);
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	y_log_message(Y_LOG_LEVEL_DEBUG, "Successfully removed project: %s", id);

	return ulfius_set_empty_response(response, HTTP_NO_CONTENT);
}

int
project_delete_file(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	char path[PATH_MAX] = {0};
	const char *id;
	const char *file;
	int rc;

	UNUSED(user_data);

	if (!(id = u_map_get(request->map_url, "id"))) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No project id was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	if (!(file = u_map_get(request->map_url, "file"))) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No file was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	rc = snprintf(path, sizeof(path), "%s/%s/%s", PROJECT_PATH, id, file);
	if (rc <= 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to generate project file path.");
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	if (unlink(path) != 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to delete project file: %s", path);
		return ulfius_set_empty_response(response, HTTP_NOT_FOUND);
	}

	return ulfius_set_empty_response(response, HTTP_NO_CONTENT);
}

int
project_get_file(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	char path[PATH_MAX] = {0};
	struct stat fstat;
	const char *id;
	const char *file;
	char *buffer;
	int fd;
	int rc;

	UNUSED(user_data);

	if (!(id = u_map_get(request->map_url, "id"))) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No project id was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	if (!(file = u_map_get(request->map_url, "file"))) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No file was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	rc = snprintf(path, sizeof(path), "%s/%s/%s", PROJECT_PATH, id, file);
	if (rc <= 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to generate project file path.");
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	if (stat(path, &fstat) == -1) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to get stats on project file: %s", path);
		return ulfius_set_empty_response(response, HTTP_NOT_FOUND);
	}

	buffer = malloc(fstat.st_size);

	if ((fd = open(path, O_RDONLY)) == -1) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to open project file: %s", path);
		rc = ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
		goto free_buffer;
	}

	if (read(fd, buffer, fstat.st_size) != fstat.st_size) {
		y_log_message(Y_LOG_LEVEL_ERROR, "Failed to read project file: %s", path);
		rc = ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
		goto close_file;
	}

	rc = ulfius_set_binary_response(response, HTTP_OK, buffer, fstat.st_size);

close_file:
	close(fd);

free_buffer:
	free(buffer);

	return rc;
}

int
project_get_files(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	char path[PATH_MAX] = {0};
	json_t *root;
	struct dirent *dir;
	const char *id;
	DIR *d;
	int rc;

	UNUSED(user_data);

	if (!(id = u_map_get(request->map_url, "id")) ||
	    strlen(id) <= 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No project id was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	rc = snprintf(path, sizeof(path), "%s/%s", PROJECT_PATH, id);
	if (rc <= 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to generate project path.");
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	if (_project_path_check(path, FALSE) != 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Tried to delete project that doesn't exist: %s", id);
		return ulfius_set_empty_response(response, HTTP_NOT_FOUND);
	}

	if (!(root = json_array()))
		return U_ERROR_MEMORY;

	if (!(d = opendir(path))) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to iterate through directory: %s", path);
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	while ((dir = readdir(d))) {
		if (dir->d_type == DT_REG)
			json_array_append_new(root, json_string(dir->d_name));
	}

	closedir(d);

	y_log_message(Y_LOG_LEVEL_DEBUG, "Files for project '%s' requested.", id);

	return ulfius_set_json_response(response, HTTP_OK, root);
}

int
project_get_list(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	json_t *root;
	struct dirent *dir;
	DIR *d;

	UNUSED(request);
	UNUSED(user_data);

	_project_path_check(PROJECT_PATH, TRUE);

	if (!(root = json_array()))
		return U_ERROR_MEMORY;

	if (!(d = opendir(PROJECT_PATH))) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to iterate through project directory.");
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	while ((dir = readdir(d))) {
		if (dir->d_type == DT_DIR &&
		    dir->d_name[0] != '.') {
			json_array_append_new(root, json_string(dir->d_name));
		}
	}

	closedir(d);

	return ulfius_set_json_response(response, HTTP_OK, root);
}

int
project_post_file(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	char path[PATH_MAX] = {0};
	struct stat ignored;
	const char *id;
	const char *file;
	int fd;
	int rc;

	UNUSED(user_data);

	if (!(id = u_map_get(request->map_url, "id"))) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No project id was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	if (!(file = u_map_get(request->map_url, "file"))) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No file was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	rc = snprintf(path, sizeof(path), "%s/%s/%s", PROJECT_PATH, id, file);
	if (rc <= 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to generate project file path.");
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	if (stat(path, &ignored) != -1) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Tried to override project file: %s", path);
		return ulfius_set_empty_response(response, HTTP_CONFLICT);
	}

	if ((fd = open(path, O_CREAT|O_WRONLY)) == -1) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to open project file for writing: %s", path);
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	if (write(fd, request->binary_body, request->binary_body_length) != (ssize_t) request->binary_body_length) {
		y_log_message(Y_LOG_LEVEL_ERROR, "Failed to write project file: %s", path);
		close(fd);
		unlink(path);
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	close(fd);

	return ulfius_set_empty_response(response, HTTP_NO_CONTENT);
}

int
project_post_new(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	char path[PATH_MAX] = {0};
	const char *id;
	int rc;

	UNUSED(user_data);

	if (!(id = u_map_get(request->map_post_body, "id")) ||
	    strlen(id) <= 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No project id was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	rc = snprintf(path, sizeof(path), "%s/%s", PROJECT_PATH, id);
	if (rc <= 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to generate project path.");
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	rc = _project_path_check(PROJECT_PATH, TRUE);
	if (rc == -1) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to create project directory.");
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	rc = _project_path_check(path, TRUE);

	/* TODO: MAYBE GENERATE TEMPLATE FILES */

	y_log_message(Y_LOG_LEVEL_DEBUG, "Project '%s' was successfully created.", id);

	return ulfius_set_empty_response(response, rc == 1 ? HTTP_CREATED : HTTP_CONFLICT);
}

int
project_put_file(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	char path[PATH_MAX] = {0};
	struct stat ignored;
	const char *id;
	const char *file;
	int fd;
	int rc;

	UNUSED(user_data);

	if (!(id = u_map_get(request->map_url, "id"))) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No project id was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	if (!(file = u_map_get(request->map_url, "file"))) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No file was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	rc = snprintf(path, sizeof(path), "%s/%s/%s", PROJECT_PATH, id, file);
	if (rc <= 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to generate project file path.");
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	if (stat(path, &ignored) == -1) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Tried to override non-existent project file: %s", path);
		return ulfius_set_empty_response(response, HTTP_NOT_FOUND);
	}

	if ((fd = open(path, O_TRUNC|O_WRONLY)) == -1) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to open project file for writing: %s", path);
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	if (write(fd, request->binary_body, request->binary_body_length) != (ssize_t) request->binary_body_length) {
		y_log_message(Y_LOG_LEVEL_ERROR, "Failed to write project file: %s", path);
		close(fd);
		unlink(path);
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	close(fd);

	return ulfius_set_empty_response(response, HTTP_NO_CONTENT);
}

/*****************************************************************************/

/* _project_path_check
 *
 * Function checks if the specified path exists.
 *
 * If the path does not exist and the 'create' parameter is set to FALSE (0),
 * the call will fail. Otherwise the function will attempt to create the path.
 *
 * RETURN VALUES
 *
 * The function will return zero (0) if the path exists and one (1) if the
 * path did not exist but was successfully created.
 *
 * If an error is encountered, the function will return -1.
 */
int
_project_path_check(const char *path, int create)
{
	struct stat ignored;
	int rc = 0;

	if (stat(path, &ignored) == -1) {
		if (create && mkdir(path, S_IRWXU) != -1)
			rc = 1;
		else
			rc = -1;
	}

	return rc;
}

