#include <signal.h>
#include "event2/event.h"
#include "BnxBot.h"

int main(int argc, char **argv) {
	struct event_base *pEventBase;

	signal(SIGPIPE, SIG_IGN);

	pEventBase = event_base_new();

	BnxBot bnxBot;

	bnxBot.SetServerAndPort("localhost");
	bnxBot.SetNickname("enslay");
	bnxBot.AddHomeChannel("#nslay");
	bnxBot.LoadResponseRules("response.txt");
	
	bnxBot.SetEventBase(pEventBase);

	bnxBot.StartUp();

	event_base_dispatch(pEventBase);

	event_base_free(pEventBase);

	return 0;
}

