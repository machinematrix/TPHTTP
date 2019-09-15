#define _GNU_SOURCE
#include <stdio.h>
#include "HttpServer.h"
#include <string.h>

void hello(HttpRequestHandle req)
{
	const char *html = "<h1 style=\"background-color:DodgerBlue;\">Hello World</h1>";
	char num[16];
	sprintf(num, "%d", strlen(html));
	//HttpServer_SendHtml(req, "<h1 style=\"background-color:DodgerBlue;\">Hello World</h1>");
	HttpResponseHandle resp = HttpServer_CreateResponse();

	HttpServer_SetResponseField(resp, HttpResponseField_Connection, "close");
	HttpServer_SetResponseField(resp, HttpResponseField_ContentType, "text/html");
	HttpServer_SetResponseField(resp, HttpResponseField_ContentLength, num);
	HttpServer_SetResponseBody(resp, html, strlen(html));
	HttpServer_SendResponse(req, resp);

	HttpServer_DestroyResponse(resp);
}

void bye(HttpRequestHandle req)
{
	HttpServer_SendHtml(req, "<h1 style=\"background-color:Tomato;\">Bye World</h1>");
}

void file(HttpRequestHandle req)
{
	HttpServer_SendHtml(req, "<img src=\"https://miro.medium.com/max/4096/1*vs239SecVBaB4HvLsZ8O5Q.png\" alt=\"Italian Trulli\">");
}

void ctx1(HttpRequestHandle req)
{
	HttpServer_SendHtml(req, "<h1 style=\"background-color:Tomato;\">ctx1</h1>");
}

void ctx2(HttpRequestHandle req)
{
	HttpServer_SendHtml(req, "<h1 style=\"background-color:Tomato;\">ctx2</h1>");
}

void ctx3(HttpRequestHandle req)
{
	HttpServer_SendHtml(req, "<h1 style=\"background-color:Tomato;\">ctx3</h1>");
}

int main(void)
{
    HttpServerHandle sv = HttpServer_Create();
	
	if (sv)
	{
		HttpServer_SetEndpointCallback(sv, "/hello", hello);
		HttpServer_SetEndpointCallback(sv, "/bye", bye);
		HttpServer_SetEndpointCallback(sv, "/files", file);
		/*HttpServer_SetEndpointCallback(sv, "/", ctx1);
		HttpServer_SetEndpointCallback(sv, "/apps", ctx2);
		HttpServer_SetEndpointCallback(sv, "/apps/foo", ctx3);*/

		HttpServer_Start(sv);

		if (HttpServer_GetStatus(sv) == Running)
		{
			int continuar = 1;

			while (continuar)
			{
				scanf("%d", &continuar);
			}
		}
		else
			printf("%s\n", HttpServer_GetErrorMessage(sv));

		HttpServer_Destroy(sv);
	}
	else printf("Could not create server\n");
    return 0;
}