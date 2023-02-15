#include <iostream>
#include "rslogging.h"
#include "pevents.h"
#include "threadpool.h"
#include <vector>
#include "ajob.h"
#include "bjob.h"
#include "cjob.h"
#include <unistd.h>

int main(int, char**) {
    g_RsLog.setLogLevel(ERsLogLevel::DEBUG);

    RSLOG_DEBUG << "Threadpool test!";
    smart_event_t event = CreateEvent(true, false);
    RSLOG_DEBUG << "new thread pool";
    TPThreadPool* pThPool = new TPThreadPool();
    RSLOG_DEBUG << "start thread pool";
    pThPool->start(4,10);
    usleep(500);


    std::vector<TPBaseJob*> vecJobs;
    TPBaseJob* pJob = NULL;
    int jobId = 0;
    for (int i = 0; i < 20; i++)
    {
        pJob = new AJob();
        pThPool->submitJob(pJob, &jobId);
        vecJobs.push_back(pJob);
        RSLOG_DEBUG << "job id = " << jobId;
    }

    WaitForEvent(event, 3*60*1000);

    RSLOG_DEBUG << "delete thread pool";
    delete pThPool;
    pThPool = NULL;
    RSLOG_DEBUG << "destroy event!";
    DestroyEvent(event);

    /*
    std::vector<TPBaseJob*>::iterator it = vecJobs.begin();
    int count = 0;
    for (; it != vecJobs.end(); it++)
    {
        delete *it;
        RSLOG_DEBUG << "delete job index = " << count++;
    }
    */
    RSLOG_DEBUG << "delete job count = " << vecJobs.size();
    vecJobs.clear();

    RSLOG_DEBUG << "exit main....";

    std::cout << "Hello, world!\n";
}
