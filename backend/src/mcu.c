#include "mcu.h"

#include <jansson.h>
#include <ulfius.h>

#include "common.h"
#include "config.h"

int
mcu_put_flash(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	UNUSED(request);
	UNUSED(response);
	UNUSED(user_data);

	return U_ERROR;
}

int
mcu_put_reset(const struct _u_request *request, struct _u_response *response, void *user_data)
{
	UNUSED(request);
	UNUSED(response);
	UNUSED(user_data);

	return U_ERROR;
}

