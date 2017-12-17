#include "cls_OvchTimer.h"

cls_OvchTimer::cls_OvchTimer(void) :
	mTimerStart((GLint64)0), mTimerIntermid((GLint64)0)
{
}

cls_OvchTimer::~cls_OvchTimer(void)
{
}

// return timestamp
double cls_OvchTimer::Start(void)
{
	glGetInteger64v(GL_TIMESTAMP, &mTimerStart);
	return (double)mTimerStart/1000000.0;
}

// return from start to current, clear the timer
double cls_OvchTimer::Stop(void)
{
	if (!mTimerStart) return 0.;
	glGetInteger64v(GL_TIMESTAMP, &mTimerIntermid);
	GLint64 res = mTimerIntermid - mTimerStart;

	mTimerStart = (GLint64)0;
	mTimerIntermid = (GLint64)0;

	return (double)res/1000000.0;
}

// return from prev to current, set intermid=current
double cls_OvchTimer::Milestone(void)
{
	GLint64 prev;
	if (mTimerIntermid) prev = mTimerIntermid;
	else prev = mTimerStart;
	if (!prev) return 0.;

	glGetInteger64v(GL_TIMESTAMP, &mTimerIntermid);
	return (double)(mTimerIntermid - prev)/1000000.0;
}

// return from start to current, set intermid=current
double cls_OvchTimer::MilestoneFromStart(void)
{
	if (!mTimerStart) return 0.;
	glGetInteger64v(GL_TIMESTAMP, &mTimerIntermid);
	return (double)(mTimerIntermid - mTimerStart)/1000000.0;
}

// return from start to current, preserve intermid
double cls_OvchTimer::FromStart(void)
{
	if (!mTimerStart) return 0.;
	GLint64 cur;
	glGetInteger64v(GL_TIMESTAMP, &cur);
	return (double)(cur - mTimerStart)/1000000.0;
}
