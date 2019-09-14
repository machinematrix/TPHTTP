#ifndef __HTTPSERVER__
#define __HTTPSERVER__

enum State
{
    Running = 1,
	Stopped
};

enum HttpResponseField
{
	HttpResponseField_Access_Control_Allow_Origin,
	HttpResponseField_Access_Control_Allow_Credentials,
	HttpResponseField_Access_Control_Expose_Headers,
	HttpResponseField_Access_Control_Max_Age,
	HttpResponseField_Access_Control_Allow_Methods,
	HttpResponseField_Access_Control_Allow_Headers,
	HttpResponseField_Accept_Patch,
	HttpResponseField_Accept_Ranges,
	HttpResponseField_Age,
	HttpResponseField_Allow,
	HttpResponseField_Alt_Svc,
	HttpResponseField_Cache_Control,
	HttpResponseField_Connection,
	HttpResponseField_Content_Disposition,
	HttpResponseField_Content_Encoding,
	HttpResponseField_Content_Language,
	HttpResponseField_Content_Length,
	HttpResponseField_Content_Location,
	HttpResponseField_Content_MD5,
	HttpResponseField_Content_Range,
	HttpResponseField_Content_Type,
	HttpResponseField_Date,
	HttpResponseField_Delta_Base,
	HttpResponseField_ETag,
	HttpResponseField_Expires,
	HttpResponseField_IM,
	HttpResponseField_Last_Modified,
	HttpResponseField_Link,
	HttpResponseField_Location,
	HttpResponseField_P3P,
	HttpResponseField_Pragma,
	HttpResponseField_Proxy_Authenticate,
	HttpResponseField_Public_Key_Pins,
	HttpResponseField_Retry_After,
	HttpResponseField_Server,
	HttpResponseField_Set_Cookie,
	HttpResponseField_Strict_Transport_Security,
	HttpResponseField_Trailer,
	HttpResponseField_Transfer_Encoding,
	HttpResponseField_Tk,
	HttpResponseField_Upgrade,
	HttpResponseField_Vary,
	HttpResponseField_Via,
	HttpResponseField_Warning,
	HttpResponseField_WWW_Authenticate,
	HttpResponseField_X_Frame_Options
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
void HttpServer_SetResponseField(HttpResponsePtr response, int field, const char *value);
void HttpServer_SetResponseBody(HttpResponsePtr response, const void *body, size_t bodyLength);

void HttpServer_SendHtml(HttpRequestPtr request, const char *html);
void HttpServer_SendResponse(HttpRequestPtr request, HttpResponsePtr response);
char* HttpServer_GetRequestUri(HttpRequestPtr request);
const char* HttpServer_GetErrorMessage(HttpServerPtr server);

#endif