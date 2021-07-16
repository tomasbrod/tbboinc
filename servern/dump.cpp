
void dumpxml(IStream& hs, bool skipped=false)
{
	const size_t cols = 32;
	XML_TAG4 doc ( &hs, "dump" );
	if(!skipped) {
		config.group.dump_id(doc);
	}
	for(const auto& dbit : config.group.dbs) {
		KVBackend* kv= dbit.second.get();
		if(kv->cfg->dump == skipped) {
			XML_TAG4 skip(doc,"skipped");
			skip.attr("db").put(dbit.first);
			continue;
		}
		std::unique_ptr<KVBackend::Iterator> it = kv->getIterator();
		CUnchStream key, val;
		while(it->Get(key,val)) {
			long oid = key.rb2();
			if((oid>>13) == 7)
			{
				XML_TAG4 tag( doc, "task" );
			}
			else if(CPlugin* plugin = config.group.getPtrO<CPlugin>(oid))
			{
				XML_TAG4( doc, plugin->type_text ).put(oid);
			}
			else if(config.group.getPtrO<GroupCtl>(oid))
			{
				//XML_TAG4( doc, "group" ).put(oid);
			}
			else
			{
				XML_TAG4 tag( doc, "unknown" );
				tag.attr("db").put(dbit.first);
				const char* type_name;
				const char* type_text = config.group.getTypeText(oid,&type_name);
				if(type_text)
					tag.attr("kind").put(type_text);
				if(type_name && *type_name)
					tag.attr("name").put(type_name);
				tag.attr("key");
				key.skip(-2);
				hs.writehex(key);
				hs.w1(' ');
				tag.in_tag=2;
				tag.body();
				while(val.length()>cols) {
					hs.w1('\n');
					hs.writehex(val.base,cols);
					val.base+=cols;
				}
				hs.writehex(val);
			}
		}
	}
}

void dumphex(IStream& hs, bool skipped=false)
{
	const size_t cols = 32;
	for(const auto& dbit : config.group.dbs) {
		KVBackend* kv= dbit.second.get();
		XML_TAG4 tagdb( &hs, string(dbit.first) );
		if(kv->cfg->dump == skipped) {
			tagdb.attr("skipped").put_bool(1);
			continue;
		}
		std::unique_ptr<KVBackend::Iterator> it = kv->getIterator();
		CUnchStream key, val;
		while(it->Get(key,val)) {
			tagdb.body(1);
			hs.w1('[');
			hs.writehex(key);
			hs.w1(']');
			if((val.length()+key.length()+2)>cols)
				hs.w1('\n');
			while(val.length()>cols) {
				hs.writehex(val.base,cols);
				val.base+=cols;
				hs.w1('\n');
			}
			hs.writehex(val);
		}
	}
}
