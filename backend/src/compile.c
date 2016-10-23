#include "compile.h"

#include <jansson.h>
#include <ulfius.h>

#include "common.h"
#include "config.h"

int
compile_get_log(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	UNUSED(request);
	UNUSED(response);
	UNUSED(user_data);

	return U_ERROR;
}

int
compile_put_project(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	UNUSED(request);
	UNUSED(response);
	UNUSED(user_data);

	return U_ERROR;
}

