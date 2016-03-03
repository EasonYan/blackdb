//#pragma comment (lib, "libuv.lib")
//#pragma comment (lib, "ws2_32.lib")
//#pragma comment(lib, "psapi.lib")
//#pragma comment(lib, "Iphlpapi.lib")
//
//#include "stdafx.h"
//#include<iostream>
//#include <string.h>
//#include <assert.h>
//#include <WinSock2.h>
//#include <Windows.h>
//
//#include "Client.h"
//#include <uv.h>
//using namespace std;
//
#pragma comment (lib, "libuv.lib")
#pragma comment (lib, "ws2_32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "Iphlpapi.lib")

#include "stdafx.h"
#include<iostream>
#include <string.h>
#include <assert.h>
#include <WinSock2.h>
#include <Windows.h>

#include <uv.h>
using namespace std;
#define ADDRESS ("127.0.0.1")
#define PORT 7000
#define MAXSIZE (1024*16)
char buffer[MAXSIZE];


typedef struct
{
	uv_write_t req;
	uv_buf_t buf;
} write_req_t;
void echo_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf);
static void after_write(uv_write_t* req, int status);
void on_connect(uv_connect_t *connection, int status);
void receiveData(uv_stream_t *client);

uv_loop_t *loop;
void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	buf->base = (char*)malloc(suggested_size + 1); // +1 for eventually append \0 for debugging
	buf->len = suggested_size;
	//log("Allocated buffer!");
}
void echo_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) 
{
	if (nread < 0)
	{
		/* Error or EOF */
		if (buf->base)
		{
			free(buf->base);
		}
		uv_close((uv_handle_t*)handle, NULL);
		return;
	}
	if (nread == 0)
	{
		free(buf->base);
		return;
	}
	buf->base[nread] = '\0';
	printf("Received: %s\n", buf->base); 
	receiveData(handle);
	return;
}
static void after_write(uv_write_t* req, int status) {

	/* Free the read/write buffer and the request */
	write_req_t* wr;
	/* Free the read/write buffer and the request */
	wr = (write_req_t*)req;
	//printf("Writed: %s\n", wr->buf.base);
	free(wr->buf.base);
	//0为成功，callback 运行
	if (status == 0)
	{
		uv_read_start(req->handle, alloc_buffer, echo_read);
	}
	else{
		fprintf(stderr, "uv_write error: %s - %s\n", uv_err_name(status), uv_strerror(status));
		if (!uv_is_closing((uv_handle_t*)req->handle))
		{
			uv_close((uv_handle_t*)req->handle, NULL);
		}
	}
}
void receiveData(uv_stream_t *client)
{

	cout << "<" << ADDRESS << ":" << PORT << ":>";
	cin.getline(buffer, MAXSIZE);
	char quit[] = "quit";
	int len = strlen(buffer) + 1;
	if (strcmp(buffer, quit) == 0)
	{
		cout << "quit client" << endl;
		uv_close((uv_handle_t*)client, NULL);
	}
	char *message = (char*)malloc(len * sizeof(char));
	memcpy(message, buffer, len);
	uv_buf_t buf = uv_buf_init(message, len);
	write_req_t  *wr;
	wr = (write_req_t *)malloc(sizeof(write_req_t));
	wr->buf = buf;
	int buf_count = 1;
	uv_write(&wr->req, client, &wr->buf, 1, after_write);
}
void on_connect(uv_connect_t *connection, int status)
{
	/*while (1)
	{*/
	if (status == -1)
	{
		fprintf(stderr, "error on_connect");
		return;
	}
	receiveData(connection->handle);
}

int main(int argc, char* argv[])
{
	loop = uv_default_loop();
	uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, client);

	uv_connect_t* connect = (uv_connect_t*)malloc(sizeof(uv_connect_t));
	struct sockaddr_in dest;
	uv_ip4_addr(ADDRESS, PORT, &dest);
	uv_tcp_connect(connect, client, (const struct sockaddr*) &dest, on_connect);
	uv_run(loop, UV_RUN_DEFAULT);
	system("pause");
	return 0;
}