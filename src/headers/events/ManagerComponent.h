/*
 * Copyright (c) 2014, webvariants GmbH, http://www.webvariants.de
 *
 * This file is released under the terms of the MIT license. You can find the
 * complete text in the attached LICENSE file or online at:
 *
 * http://www.opensource.org/licenses/mit-license.php
 *
 * @author: Christian Sonderfeld (christian.sonderfeld@webvariants.de)
 */


#ifndef __EVENTMANAGERCOMPONENT__
#define __EVENTMANAGERCOMPONENT__

#include "events/Manager.h"
#include "world/Component.h"

namespace Susi {
namespace Events {

class ManagerComponent : Manager, Susi::System::Component
{
protected:
	std::atomic<bool> noPublish;
	std::mutex mutex;
public:
	ManagerComponent(size_t workers = 4, size_t buffsize = 32):Manager{workers, buffsize},noPublish{false}{}

	void publish(EventPtr event, Consumer finishCallback = Consumer{}) override {
		if(noPublish.load()) {
			std::shared_ptr<Event> sharedEvent{event.release()};
			finishCallback(sharedEvent);
			return;
		}
		Manager::publish(std::move(event), finishCallback);
	}

	virtual void start() override {}

	virtual void stop() override {
		if(!noPublish.load()) {
			noPublish.store(true);
			std::unique_lock<std::mutex> lk(mutex);
			publishFinished.wait(lk,[this](){return manager->publishProcesses.size() == 0;});
		}
	}

	~ManagerComponent() {stop();}
};

}
}

#endif // __EVENTMANAGERCOMPONENT__