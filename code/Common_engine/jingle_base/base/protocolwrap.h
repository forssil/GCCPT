#ifndef _WRAP_PROTOCOL_
#define _WRAP_PROTOCOL_

namespace talk_base{

	enum ProtocolType {
		PROTO_UDP,
		PROTO_TCP,
		PROTO_SSLTCP,
		PROTO_LAST = PROTO_SSLTCP
	};

}// namespace talk_base


#endif //_WRAP_PROTOCOL_