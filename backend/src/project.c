#include "project.h"

#include <sys/stat.h>

#include <dirent.h>
#include <fcntl.h>
#include <jansson.h>
#include <ulfius.h>

#include "common.h"
#include "config.h"

static int _project_path_check(const char *, int);

int
project_delete_existing(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	char path[PATH_MAX] = {0};
	struct dirent *dentry;
	const char *id;
	DIR *dh;
	int rc;

	UNUSED(user_data);

	id = u_map_get(request->map_url, "id");
	if(!id) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No project id was specified.");
		rc = HTTP_BAD_REQUEST;
		goto finish_response;
	}

	rc = snprintf(path, sizeof(path), "%s/%s", PROJECT_PATH, id);
	if (rc <= 0) {
		y_log_message(Y_LOG_LEVEL_ERROR, "snprintf failed: %s/%s", PROJECT_PATH, id);
		rc = HTTP_INTERNAL_SERVER_ERROR;
		goto finish_response;
	}

	if (_project_path_check(path, FALSE) != 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Tried to delete project that doesn't exist: %s", id);
		rc = HTTP_NOT_FOUND;
		goto finish_response;
	}

	dh = opendir(path);
	if (!dh) {
		y_log_message(Y_LOG_LEVEL_ERROR, "opendir failed: %s", path);
		rc = HTTP_INTERNAL_SERVER_ERROR;
		goto finish_response;
	}

	/* currently not supporting multiple directory levels */

	while ((dentry = readdir(dh))) {
		switch (dentry->d_type) {
		case DT_REG:
		case DT_LNK:
			if (unlinkat(dirfd(dh), dentry->d_name, 0) == -1) {
				y_log_message(Y_LOG_LEVEL_DEBUG,
				    "Failed to delete file in project '%s': %s",
				    id, dentry->d_name);
			}
			break;
		case DT_DIR:
			if (strcmp(dentry->d_name, ".") == 0 ||
			    strcmp(dentry->d_name, "..") == 0) {
				break;
			}

			/* fall-through */
		default:
			y_log_message(Y_LOG_LEVEL_DEBUG,
			    "Unsupported file uncounted in project '%s': %s",
			    id, dentry->d_name);
			break;
		}
	}

	closedir(dh);

	if (rmdir(path) == -1) {
		y_log_message(Y_LOG_LEVEL_ERROR, "rmdir failed: %s", path);
		rc = HTTP_INTERNAL_SERVER_ERROR;
		goto finish_response;
	}

	y_log_message(Y_LOG_LEVEL_DEBUG, "Project '%s' was successfully deleted.", id);

	rc = HTTP_NO_CONTENT;

finish_response:
	return ulfius_set_empty_response(response, rc);
}

int
project_delete_file(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	char path[PATH_MAX] = {0};
	const char *id;
	const char *file;
	int rc;

	UNUSED(user_data);

	id = u_map_get(request->map_url, "id");
	if (!id) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No project id was specified.");
		rc = HTTP_BAD_REQUEST;
		goto finish_response;
	}

	file = u_map_get(request->map_url, "file");
	if (!file) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No file was specified.");
		rc = HTTP_BAD_REQUEST;
		goto finish_response;
	}

	rc = snprintf(path, sizeof(path), "%s/%s/%s", PROJECT_PATH, id, file);
	if (rc <= 0) {
		y_log_message(Y_LOG_LEVEL_ERROR, "snprintf failed: %s/%s/%s",
		    PROJECT_PATH, id, file);
		rc = HTTP_INTERNAL_SERVER_ERROR;
		goto finish_response;
	}

	if (unlink(path) == -1) {
		y_log_message(Y_LOG_LEVEL_ERROR, "unlink failed: %s", path);
		rc = HTTP_NOT_FOUND;
		goto finish_response;
	}

	y_log_message(Y_LOG_LEVEL_DEBUG, "Successfully deleted project file: %s", path);

	rc = HTTP_NO_CONTENT;

finish_response:
	return ulfius_set_empty_response(response, rc);
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

	id = u_map_get(request->map_url, "id");
	if (!id) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No project id was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	file = u_map_get(request->map_url, "file");
	if (!file) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No file was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	if (file[0] == '.') {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Invalid file name specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	rc = snprintf(path, sizeof(path), "%s/%s/%s", PROJECT_PATH, id, file);
	if (rc <= 0) {
		y_log_message(Y_LOG_LEVEL_ERROR, "snprintf failed: %s/%s/%s",
		    PROJECT_PATH, id, file);
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	if (stat(path, &fstat) == -1) {
		y_log_message(Y_LOG_LEVEL_ERROR, "stat failed: %s", path);
		return ulfius_set_empty_response(response, HTTP_NOT_FOUND);
	}

	buffer = malloc(fstat.st_size);
	if (!buffer) {
		y_log_message(Y_LOG_LEVEL_ERROR, "malloc failed: %s", path);
		return U_ERROR_MEMORY;
	}

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		y_log_message(Y_LOG_LEVEL_ERROR, "open failed: %s", path);
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
	struct dirent *dentry;
	const char *id;
	DIR *dh;
	int rc;

	UNUSED(user_data);

	id = u_map_get(request->map_url, "id");
	if (!id || strlen(id) <= 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No project id was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	rc = snprintf(path, sizeof(path), "%s/%s", PROJECT_PATH, id);
	if (rc <= 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to calculate project path.");
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	if (_project_path_check(path, FALSE) != 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Tried to delete project that doesn't exist: %s", id);
		return ulfius_set_empty_response(response, HTTP_NOT_FOUND);
	}

	root = json_array();
	if (!root)
		return U_ERROR_MEMORY;

	if (!(dh = opendir(path))) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to iterate through directory: %s", path);
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	while ((dentry = readdir(dh))) {
		if (dentry->d_type == DT_REG &&
		    dentry->d_name[0] != '.')
			json_array_append_new(root, json_string(dentry->d_name));
	}

	closedir(dh);

	y_log_message(Y_LOG_LEVEL_DEBUG, "Files for project '%s' requested.", id);

	return ulfius_set_json_response(response, HTTP_OK, root);
}

int
project_get_list(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	json_t *root;
	struct dirent *dentry;
	DIR *dh;

	UNUSED(request);
	UNUSED(user_data);

	_project_path_check(PROJECT_PATH, TRUE);

	if (!(root = json_array()))
		return U_ERROR_MEMORY;

	if (!(dh = opendir(PROJECT_PATH))) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to iterate through project directory.");
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	while ((dentry = readdir(dh))) {
		if (dentry->d_type == DT_DIR &&
		    dentry->d_name[0] != '.') {
			json_array_append_new(root, json_string(dentry->d_name));
		}
	}

	closedir(dh);

	return ulfius_set_json_response(response, HTTP_OK, root);
}

int
project_post_file(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	char path[PATH_MAX] = {0};
	struct stat fstat;
	const char *id;
	const char *file;
	int fd;
	int rc;

	UNUSED(user_data);

	id = u_map_get(request->map_url, "id");
	if (!id) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No project id was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	file = u_map_get(request->map_url, "file");
	if (!file) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No file was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	if (file[0] == '.') {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Invalid file name specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	rc = snprintf(path, sizeof(path), "%s/%s/%s", PROJECT_PATH, id, file);
	if (rc <= 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to calculate project file path.");
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	if (stat(path, &fstat) != -1) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Tried to override project file: %s", path);
		return ulfius_set_empty_response(response, HTTP_CONFLICT);
	}

	fd = open(path, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if (fd == -1) {
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
	struct dirent *dentry;
	DIR *dh_path;
	DIR *dh_skel;
	int rc;

	UNUSED(user_data);

	id = u_map_get(request->map_post_body, "id");
	if (!id || strlen(id) <= 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No project id was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	rc = snprintf(path, sizeof(path), "%s/%s", PROJECT_PATH, id);
	if (rc <= 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to calculate project path.");
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	rc = _project_path_check(PROJECT_PATH, TRUE);
	if (rc == -1) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to create root projects directory.");
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	rc = _project_path_check(path, TRUE);
	if (rc != 1)
		return ulfius_set_empty_response(response, HTTP_CONFLICT);

	if (!(dh_path = opendir(path))) {
		y_log_message(Y_LOG_LEVEL_ERROR,
		    "Failed to open project directory: %s", path);
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	if (!(dh_skel = opendir(SKEL_PATH))) {
		y_log_message(Y_LOG_LEVEL_ERROR,
		    "Failed to iterate through skel directory: %s",
		    SKEL_PATH);
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	while ((dentry = readdir(dh_skel))) {
		switch (dentry->d_type) {
		case DT_REG:
		case DT_LNK:
			rc = linkat(dirfd(dh_skel), dentry->d_name,
				    dirfd(dh_path), dentry->d_name, AT_SYMLINK_FOLLOW);
			if (rc == -1) {
				y_log_message(Y_LOG_LEVEL_ERROR,
				    "Failed to copy skel directory file: %s",
				    dentry->d_name);
			}
			break;

		default: break;
		}
	}

	closedir(dh_skel);
	closedir(dh_path);

	y_log_message(Y_LOG_LEVEL_DEBUG, "Project '%s' was successfully created.", id);

	return ulfius_set_empty_response(response, HTTP_CREATED);
}

int
project_put_file(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	char path[PATH_MAX] = {0};
	struct stat fstat;
	const char *id;
	const char *file;
	int fd;
	int rc;

	UNUSED(user_data);

	id = u_map_get(request->map_url, "id");
	if (!id) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No project id was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	file = u_map_get(request->map_url, "file");
	if (!file) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No file was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	if (file[0] == '.') {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Invalid file name specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	rc = snprintf(path, sizeof(path), "%s/%s/%s", PROJECT_PATH, id, file);
	if (rc <= 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to calculate project file path.");
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	if (stat(path, &fstat) == -1) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Tried to override non-existent project file: %s", path);
		return ulfius_set_empty_response(response, HTTP_NOT_FOUND);
	}

	fd = open(path, O_TRUNC|O_WRONLY);
	if (fd == -1) {
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
	struct stat fstat;
	int rc = 0;

	if (stat(path, &fstat) == -1) {
		if (create && mkdir(path, S_IRWXU) != -1)
			rc = 1;
		else
			rc = -1;
	}

	return rc;
}

