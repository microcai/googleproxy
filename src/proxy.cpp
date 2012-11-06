/*
 * proxy.cpp
 *
 *  Created on: 2012-8-10
 *      Author: cai
 */
#include <config.h>
#include <unistd.h>
#include <string.h>
#include <boost/array.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/bind.hpp>

#ifdef ENABLE_THREAD
#	include <boost/thread.hpp>
#	undef ASIO_DISABLE_THREADS
#else
#	define ASIO_DISABLE_THREADS
#endif

//#define USE_BOOST_ASIO

#ifdef USE_BOOST_ASIO
#include <boost/asio.hpp>
typedef	boost::system::error_code error_code;
namespace asio = boost::asio;
#else
#include <asio.hpp>
typedef	asio::error_code	error_code;
#endif

#include "sd-daemon.h"

typedef boost::shared_ptr<asio::ip::tcp::socket> socketptr;
typedef boost::shared_ptr<boost::array<char,4096>> arraybuffer;

static void splice_write_handler(
	socketptr writesocket,socketptr readsocket,
	arraybuffer writebuffer,arraybuffer readbuffer,
	std::size_t bytes_transferred,
	const error_code& error)
{
	if(error){
		//终止连接
	}
}

static void splice_read_handler(
	socketptr readsocket,socketptr writesocket,
	arraybuffer  readbuffer,arraybuffer  writebuffer,
	std::size_t bytes_transferred,      /* Number of bytes read.*/
	const error_code& error /* Result of operation.*/)
{
	std::swap(readbuffer,writebuffer);
	//readbuffer.swap(writebuffer);
	//std::swap(readbuffer,writebuffer);
	if(!error){
		readsocket->async_read_some(asio::buffer(*readbuffer),
			boost::bind(&splice_read_handler,readsocket,writesocket,readbuffer,writebuffer,asio::placeholders::bytes_transferred,asio::placeholders::error)
		);
		writesocket->async_write_some(asio::buffer(*writebuffer,bytes_transferred),
			boost::bind(&splice_write_handler,writesocket,readsocket,writebuffer,readbuffer,asio::placeholders::bytes_transferred,asio::placeholders::error)
		);
	}else{
		//终止连接
	}
}

static void splicer_remote_connected(
	socketptr remotesocket,
	socketptr clientsocket,
	const error_code& error)
{
	if (!error){
		// start reading for both side
		arraybuffer read_data_remote(new boost::array<char,4096>()),
					read_data_client(new boost::array<char,4096>());

		arraybuffer write_data_remote(new boost::array<char,4096>()),
					write_data_client(new boost::array<char,4096>());

		remotesocket->async_read_some(asio::buffer(*read_data_remote,read_data_remote->size()),
			boost::bind(splice_read_handler,
						remotesocket,
						clientsocket,
						read_data_remote,write_data_remote,
						asio::placeholders::bytes_transferred,
						asio::placeholders::error));

		clientsocket->async_read_some(asio::buffer(*read_data_client,read_data_client->size()),
			boost::bind(splice_read_handler,
						clientsocket,
						remotesocket,
						read_data_client,write_data_client,
						asio::placeholders::bytes_transferred,
						asio::placeholders::error));
	}
}

static void splicer_accept_handler(asio::io_service & service,asio::ip::tcp::acceptor & httpsacceptor,
								socketptr clientsocket,
								const error_code& error,
								asio::ip::tcp::endpoint remote_splicer
								)
{
	if (!error)
	{
		// Accept succeeded.
		// do some thing with clientsocket
		// fact is , splice

		// connecting to google
		socketptr togoogle(new asio::ip::tcp::socket(service));

		std::cout << "connecting https://www.google.com/" << std::endl;

		togoogle->async_connect(remote_splicer,
				[togoogle,clientsocket](const error_code& error){
					splicer_remote_connected(togoogle,clientsocket,error);
					std::cout << "https://www.google.com/ connected" << std::endl;
				}				
		);
	}

	// again
	socketptr newclientsocket( new asio::ip::tcp::socket(service));
	httpsacceptor.async_accept(*newclientsocket,
			boost::bind(splicer_accept_handler,
						boost::ref(service),boost::ref(httpsacceptor),
						newclientsocket,asio::placeholders::error,
						remote_splicer
			)
	);
}


static void http_read_request_handler(
	asio::io_service & service,asio::ip::tcp::acceptor & httpacceptor,
	socketptr clientsocket,
	boost::shared_ptr<boost::array<char,1500>>  buffer,
	std::size_t bytes_transferred,          /* Number of bytes read.*/
	const error_code& error /* Result of operation.*/)
{
	boost::array<char,8192> url;

	/* 解析 url */
	sscanf(buffer->cbegin(),std::string(boost::str(boost::format("GET /%%%d[^ ]") % url.size())).c_str(),url.c_array());

	puts(url.cbegin());

	if(strncmp(url.cbegin(),"imgres?",7)){
		const char * page_template =
				"HTTP/1.1 301 Moved Permanently\r\n"
				"Location: https://www.google.com/%s\r\n"
				"\r\n\r\n";

		std::string	page = boost::str( boost::format(page_template) % url.c_array() );
		//写入
		clientsocket->async_write_some(asio::buffer(page.data(),page.length()),
					[clientsocket](const error_code& error,std::size_t bytes_transferred){
						// clean up
						clientsocket->close();
					});
	}else{
		
		socketptr remotesocket(new asio::ip::tcp::socket(service));

		std::cout << "connecting http://www.google.com/imgres?" << std::endl;

		remotesocket->async_connect(
			asio::ip::tcp::endpoint(asio::ip::address::from_string(GOOGLE_ADDRESS),80),
			[remotesocket,clientsocket,buffer,bytes_transferred](const error_code& error){
				if (!error){
					remotesocket->async_write_some(
						asio::buffer(*buffer,bytes_transferred),
						[remotesocket,clientsocket](const error_code& error,std::size_t bytes_transferred)
						{
							splicer_remote_connected(remotesocket,clientsocket,error);
							std::cout << "http://www.google.com/imgres? connected " << std::endl;
						}
					);
				}
			}
		);
	}
}

static void http_accept_handler(asio::io_service & service,asio::ip::tcp::acceptor & httpacceptor,
								socketptr clientsocket,
								const error_code& error)
{
	if (!error)
	{
		// Accept succeeded.
		// do some thing with clientsocket

		boost::shared_ptr<boost::array<char,1500>>  buffer(new boost::array<char,1500>());

		clientsocket->async_read_some(asio::buffer(*buffer,buffer->size()),
										boost::bind(http_read_request_handler,
													boost::ref(service),
													boost::ref(httpacceptor),
													clientsocket,buffer,
													asio::placeholders::bytes_transferred,
													asio::placeholders::error
										)
		);
	}

	// again
	socketptr newclientsocket( new asio::ip::tcp::socket(service));
	httpacceptor.async_accept(*newclientsocket,boost::bind(http_accept_handler,boost::ref(service),boost::ref(httpacceptor),newclientsocket,asio::placeholders::error));
}

int main(int argc,char *argv[])
{
	uint baseport = 0;
	if(argc == 2){
		baseport = boost::lexical_cast<uint>(argv[1]);
	}

	asio::io_service service;

	asio::ip::tcp::acceptor httpacceptor = [&]{
#if HAVE_SYSTEMD
		if(sd_listen_fds(0)>0){			
			return asio::ip::tcp::acceptor(service,asio::ip::tcp::v6(),(asio::ip::tcp::acceptor::native_type)SD_LISTEN_FDS_START);
		}
		else
#endif
			return asio::ip::tcp::acceptor(service,asio::ip::tcp::endpoint(asio::ip::tcp::v6(), 80 + baseport));
	}();

	asio::ip::tcp::acceptor httpsacceptor = [&]{
#if HAVE_SYSTEMD
		if(sd_listen_fds(0)>0){
			return asio::ip::tcp::acceptor(service,asio::ip::tcp::v6(),(asio::ip::tcp::acceptor::native_type)SD_LISTEN_FDS_START+1);
		}
		else
#endif
			return asio::ip::tcp::acceptor(service,asio::ip::tcp::endpoint(asio::ip::tcp::v6(), 443 + baseport));
	}();

	socketptr httpsclientsocket( new asio::ip::tcp::socket(service));
	httpsacceptor.async_accept(*httpsclientsocket,
			boost::bind(splicer_accept_handler,
						boost::ref(service),
						boost::ref(httpsacceptor),
						httpsclientsocket,
						asio::placeholders::error,
						asio::ip::tcp::endpoint(asio::ip::address::from_string(GOOGLE_ADDRESS),443)
			)
	);

	socketptr httpclientsocket( new asio::ip::tcp::socket(service));
	httpacceptor.async_accept(*httpclientsocket,
			boost::bind(&http_accept_handler,
						boost::ref(service),
						boost::ref(httpacceptor),
						httpclientsocket,
						asio::placeholders::error
			)
	);

#ifdef ENABLE_THREAD
	int numofcpu = sysconf(_SC_NPROCESSORS_CONF);
	if(numofcpu > 1){
		std::cout << "cpu number is " << numofcpu << ", so run that much thread" << std::endl;
		for(int i = 0 ; i < numofcpu - 1; i++)
			boost::thread(boost::bind(&asio::io_service::run,&service)).detach();
	}
#endif
	service.run();
}
