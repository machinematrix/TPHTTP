#ifndef __HTTPSERVER__
#define __HTTPSERVER__

enum States
{
    Running = 1,
	Stopped
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

void HttpServer_SendHtml(HttpRequestPtr request, const char *html);
void HttpServer_SendResponse(HttpRequestPtr request, HttpResponsePtr response);
char* HttpServer_GetRequestUri(HttpRequestPtr request);
const char* HttpServer_GetErrorMessage(HttpServerPtr server);

#endif