#define _GNU_SOURCE
#include <stdio.h>
#include "HttpServer.h"

void hello(HttpRequestPtr req)
{
	HttpServer_SendHtml(req, "<h1 style=\"background-color:DodgerBlue;\">Hello World</h1>");
}

void bye(HttpRequestPtr req)
{
	HttpServer_SendHtml(req, "<h1 style=\"background-color:Tomato;\">Bye World</h1>");
}

void file(HttpRequestPtr req)
{
	HttpServer_SendHtml(req, "<img src=\"https://miro.medium.com/max/4096/1*vs239SecVBaB4HvLsZ8O5Q.png\" alt=\"Italian Trulli\">");
}

void ctx1(HttpRequestPtr req)
{
	HttpServer_SendHtml(req, "<h1 style=\"background-color:Tomato;\">ctx1</h1>");
}

void ctx2(HttpRequestPtr req)
{
	HttpServer_SendHtml(req, "<h1 style=\"background-color:Tomato;\">ctx2</h1>");
}

void ctx3(HttpRequestPtr req)
{
	HttpServer_SendHtml(req, "<h1 style=\"background-color:Tomato;\">ctx3</h1>");
}

int main(void)
{
    HttpServerPtr sv = HttpServer_Create();
	
	if (sv)
	{
		/*HttpServer_SetEndpointCallback(sv, "/hello", hello);
		HttpServer_SetEndpointCallback(sv, "/bye", bye);
		HttpServer_SetEndpointCallback(sv, "/files", file);*/
		HttpServer_SetEndpointCallback(sv, "/", ctx1);
		HttpServer_SetEndpointCallback(sv, "/apps", ctx2);
		HttpServer_SetEndpointCallback(sv, "/apps/foo", ctx3);

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