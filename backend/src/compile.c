#include "compile.h"

#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <jansson.h>
#include <ulfius.h>

#include "common.h"
#include "config.h"

static ssize_t _stream_log(void *, uint64_t, char *, size_t);
static void _stream_log_free(void *);

int
compile_put_project(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	char path[PATH_MAX] = {0};
	struct stat fstat;
	const char *id;
	int *fd;
	int rc;

	UNUSED(user_data);

	id = u_map_get(request->map_url, "id");
	if (!id) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "No project id was specified.");
		return ulfius_set_empty_response(response, HTTP_BAD_REQUEST);
	}

	rc = snprintf(path, sizeof(path), "%s/%s", PROJECT_PATH, id);
	if (rc <= 0) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Failed to generate project file path.");
		return ulfius_set_empty_response(response, HTTP_INTERNAL_SERVER_ERROR);
	}

	if (stat(path, &fstat) == -1) {
		y_log_message(Y_LOG_LEVEL_DEBUG, "Project path doesn't exist: %s", path);
		return ulfius_set_empty_response(response, HTTP_NOT_FOUND);
	}

	fd = calloc(2, sizeof(int));

	if (-1 == pipe(fd)) {
		perror("pipe");
		return U_ERROR;
	}

	fcntl(fd[0], F_SETFD, FD_CLOEXEC);
	fcntl(fd[1], F_SETFD, FD_CLOEXEC);
	fcntl(fd[0], F_SETFL, O_NONBLOCK);

	switch (fork()) {
	case -1:
		y_log_message(Y_LOG_LEVEL_ERROR, "Failed to fork process!");
		return U_ERROR;
	case 0:
		while (-1 == dup2(fd[1], STDOUT_FILENO) && EINTR == errno);

		if (-1 == chdir(path)) {
		    perror("execl");
		    _exit(1);
		}

		execl("/usr/bin/make", "make", "clean", "all", NULL);

		perror("execl");
		_exit(1);
	}

	close(fd[1]);

	y_log_message(Y_LOG_LEVEL_DEBUG, "Child process created.");

	return ulfius_set_stream_response(response, HTTP_OK, _stream_log, _stream_log_free, -1, 1024, fd);
}

/*****************************************************************************/

ssize_t
_stream_log(void *stream_user_data, uint64_t offset, char *out_buf, size_t max)
{
	int *fd = stream_user_data;
	ssize_t bread;

	UNUSED(offset);

	bread = read(fd[0], out_buf, max);
	switch (bread) {
	case -1:
		if (EAGAIN == errno)
			return 0;

		y_log_message(Y_LOG_LEVEL_ERROR, "Failed to read from child stdout.");

		/* fall-through */

	case 0: return ULFIUS_STREAM_END;
	}

	y_log_message(Y_LOG_LEVEL_DEBUG, "Read %ld bytes from child output.", bread);

	return bread;
}

void
_stream_log_free(void * stream_user_data)
{
	int *fd = stream_user_data;
	close(fd[0]);
	free(fd);
}

