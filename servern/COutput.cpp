
using std::string;
using std::unique_ptr;
class CPlugin;

#include "build/config-output.hpp"

/* represents <output> element of config */
class COutput : public t_config_output
{
	/* Bound to plugin for event queue */
	CPlugin* plugin;

	public:
  void init(CPlugin* iplugin, GroupCtl& group)
  {
		CLog log (name);
		bind(report_db, group, log);
		bind(trickle_db, group, log);
		for( auto& el : filesv ) {
			if(el.has_db) {
				bind(el.db, group, log);
				assert(el.path.empty());
			}
			else assert(!el.path.empty());
		}
	}
};

#include "build/config-plugin.hpp"

/* represents <plugin> element of config */
class CPlugin : public t_config_plugin
{
	GroupCtl* group;
	short ID;
	// split the plugin into two parts: queues and listen
	// this thing manages the queues
	public:

	// a taskID should be added to the validation queue, because something
	// deemed inportant by <output> happened to it
	void addValidate();

	// add taskID to the expiration queue
	void setExpire(int id, int prev, int next);

	void init(GroupCtl& igroup)
  {
		group=&igroup;
		ID=group->getID("plugin", this->name, 4);
	}
};


