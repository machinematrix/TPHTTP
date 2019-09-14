#ifndef __HTTPSERVER__
#define __HTTPSERVER__

enum State
{
    Running = 1,
	Stopped
};

enum HttpResponseField
{
	HttpResponseField_Connection,
	HttpResponseField_Content_Encoding,
	HttpResponseField_Content_Language,
	HttpResponseField_Content_Length,
	HttpResponseField_Content_Location,
};

typedef struct HttpServer *HttpServerPtr;
typedef struct HttpRequest *HttpRequestPtr;
typedef struct HttpResponse *HttpResponsePtr;
typedef void(*HandlerCallback)(HttpRequestPtr);

HttpServerPtr HttpServer_Create();
void HttpServer_Start(HttpServerPtr server);
void HttpServer_Destroy(HttpServerPtr server);
void HttpServer_SetEndpointCallback(HttpServerPtr server, const char *resource, HandlerCallback callback);
int HttpServer_GetStatus(HttpServerPtr server);

HttpResponsePtr HttpServer_CreateResponse();
void HttpServer_DestroyResponse(HttpResponsePtr response);

void HttpServer_SendHtml(HttpRequestPtr request, const char *html);
void HttpServer_SendResponse(HttpRequestPtr request, HttpResponsePtr response);
char* HttpServer_GetRequestUri(HttpRequestPtr request);
const char* HttpServer_GetErrorMessage(HttpServerPtr server);

#endif