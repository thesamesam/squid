#include "squid.h"
#include "MsgPipe.h"
#include "MsgPipeSource.h"
#include "MsgPipeSink.h"
#include "MsgPipeData.h"

#include "LeakFinder.h"
LeakFinder *MsgPipeLeaker = new LeakFinder;

CBDATA_CLASS_INIT(MsgPipe);

// static event callback template
// XXX: refcounting needed to make sure destination still exists
#define MsgPipe_MAKE_CALLBACK(callName, destination) \
static \
void MsgPipe_send ## callName(void *p) { \
	MsgPipe *pipe = static_cast<MsgPipe*>(p); \
	if (pipe && pipe->canSend(pipe->destination, #callName, false)) \
		pipe->destination->note##callName(pipe); \
}

// static event callbacks
MsgPipe_MAKE_CALLBACK(SourceStart, sink)
MsgPipe_MAKE_CALLBACK(SourceProgress, sink)
MsgPipe_MAKE_CALLBACK(SourceFinish, sink)
MsgPipe_MAKE_CALLBACK(SourceAbort, sink)
MsgPipe_MAKE_CALLBACK(SinkNeed, source)
MsgPipe_MAKE_CALLBACK(SinkAbort, source)


MsgPipe::MsgPipe(const char *aName): name(aName),
        data(NULL), source(NULL), sink(NULL)
{
    leakAdd(this, MsgPipeLeaker);
}

MsgPipe::~MsgPipe()
{
    delete data;
    delete source;
    delete sink;
    leakFree(this, MsgPipeLeaker);
};

void MsgPipe::sendSourceStart()
{
    leakTouch(this, MsgPipeLeaker);
    debug(99,5)("MsgPipe::sendSourceStart() called\n");
    sendLater("SourceStart", &MsgPipe_sendSourceStart, sink);
}



void MsgPipe::sendSourceProgress()
{
    leakTouch(this, MsgPipeLeaker);
    debug(99,5)("MsgPipe::sendSourceProgress() called\n");
    sendLater("SourceProgress", &MsgPipe_sendSourceProgress, sink);
}

void MsgPipe::sendSourceFinish()
{
    leakTouch(this, MsgPipeLeaker);
    debug(99,5)("MsgPipe::sendSourceFinish() called\n");
    sendLater("sendSourceFinish", &MsgPipe_sendSourceFinish, sink);
    source = NULL;
}

void MsgPipe::sendSourceAbort()
{
    leakTouch(this, MsgPipeLeaker);
    debug(99,5)("MsgPipe::sendSourceAbort() called\n");
    sendLater("SourceAbort", &MsgPipe_sendSourceAbort, sink);
    source = NULL;
}


void MsgPipe::sendSinkNeed()
{
    leakTouch(this, MsgPipeLeaker);
    debug(99,5)("MsgPipe::sendSinkNeed() called\n");
    sendLater("SinkNeed", &MsgPipe_sendSinkNeed, source);
}

void MsgPipe::sendSinkAbort()
{
    leakTouch(this, MsgPipeLeaker);
    debug(99,5)("MsgPipe::sendSinkAbort() called\n");
    sendLater("SinkAbort", &MsgPipe_sendSinkAbort, source);
    sink = NULL;
}

void MsgPipe::sendLater(const char *callName, EVH * handler, MsgPipeEnd *destination)
{
    leakTouch(this, MsgPipeLeaker);

    if (canSend(destination, callName, true))
        eventAdd(callName, handler, this, 0.0, 0, true);
}

bool MsgPipe::canSend(MsgPipeEnd *destination, const char *callName, bool future)
{
    leakTouch(this, MsgPipeLeaker);
    const bool res = destination != NULL;
    const char *verb = future ?
                       (res ? "will send " : "wont send ") :
                               (res ? "sends " : "ignores ");
    debugs(99,5, "MsgPipe " << name << "(" << this << ") " <<
           verb << callName << " to the " <<
           (destination ? destination->kind() : "destination") << "(" <<
           destination << "); " <<
           "data: " << data << "; source: " << source << "; sink " << sink);
    return res;
}
