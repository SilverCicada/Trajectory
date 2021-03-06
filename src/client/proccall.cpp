#include "../../header/client/client.h"
#include "../../header/client/states.h"
#include "../../header/client/userio.h"
#include "../../header/client/netio.h"
#include "../../header/client/render.h"
#include "../../header/client/game.h"

std::unordered_map<ThreadId, CallMap> Dispatcher::LpcMap =
{
	//
	{
		ThreadId::C, 
		{
			{
				"Launch", [](auto _)
				{
					clog("Launch end, your strength:{}", GameInfo::instance()->Strength);

				},
			}
		}
	},

	// 
	{
		ThreadId::N,
		{
			{
				"SayHello", [](std::optional<ArgsPack> pack)
				{
					assert(NetIO::instance()->State->in_state(state::net::ToLoginServ::instance()));
					static json hloapdx{};	// appendix of 'Hello' json  
					static std::once_flag flag{};

					// init selfinfo once 
					std::call_once(flag, []()
						{
							// load self infomation from configuration 
							auto& self = hloapdx;
							const sol::table& cfg = Client::configer()["Config"]["Client"]["SelfDescriptor"];
							self["Name"] = cfg["Name"].get<std::string>();
							self["Weapon"] = cfg["Weapon"].get<std::string>();
							const auto& style = cfg["Style"].get<sol::table>();
							for (auto& [k, v] : style)
								self[k.as<std::string>()] = v.as<int>();
						});

					NetIO::instance()
							->Conn
							->channel
							->write(Protocol::LoginBuilder::make()
									.deal_type("Hello")
									.deal_subtype("Hello")
									.deal_appendix(hloapdx)
									.build());
				}
			},

			{
				"RequestRooms", [](std::optional<ArgsPack> pack)
				{
					assert(NetIO::instance()->State->in_state(state::net::ToLoginServ::instance()));
					
					NetIO::instance()
						->Conn
						->channel
						->write(Protocol::LoginBuilder::make()
								.deal_type("Request")
								.deal_subtype("Room")
								.build());
				}
			},

			{
				"ReqSelfRoomInfo", [](std::optional<ArgsPack> pack)
				{
					auto& roominfo = *GameInfo::instance()->RoomInfo;
					json apdx{};
					apdx["RoomId"] = roominfo["Id"].get<int>();

					NetIO::instance()
						->Conn
						->channel
						->write(Protocol::LoginBuilder::make()
								.deal_type("Request")
								.deal_subtype("SelfRoom")
								.deal_appendix(std::move(apdx))
								.build());
				}
			},

			{
				"CreateRoom", [](std::optional<ArgsPack> pack)
				{
					assert(NetIO::instance()->State->in_state(state::net::ToLoginServ::instance()));
					
					// rd: room desctiptor, require two field: "Name", "Area"
					json rd = std::any_cast<json>(pack.value()->args_pack().front());

					NetIO::instance()
						->Conn
						->channel
						->write(Protocol::LoginBuilder::make()
								.deal_type("Order")
								.deal_subtype("CreateRoom")
								.deal_appendix(rd)
								.build());
				}
			},

			{
				"JoinRoom", [](std::optional<ArgsPack> pack)
				{
					assert(NetIO::instance()->State->in_state(state::net::ToLoginServ::instance()));

					int id = std::any_cast<int>(pack.value()->args_pack().front());
					json apdx;
					apdx["TargetId"] = id;

					NetIO::instance()
						->Conn
						->channel
						->write(Protocol::LoginBuilder::make()
								.deal_type("Order")
								.deal_subtype("JoinRoom")
								.deal_appendix(apdx)
								.build());
				}
			},

			{
				"StartGame", [](std::optional<ArgsPack> pack)
				{
					const json& room = *GameInfo::instance()->RoomInfo;
					
					json apdx;
					apdx["BtlSevAddr"] = room["SevAddr"].get<std::string>();
					apdx["BattleId"] = room["Id"].get<int>();
					apdx["Name"] = room["Name"].get<std::string>();
					NetIO::instance()
						->Conn
						->channel
						->write(Protocol::LoginBuilder::make()
								.deal_type("Order")
								.deal_subtype("StartGame")
								.deal_appendix(apdx)
								.build());
				}
			},

			{
				"ReqResources", [](std::optional<ArgsPack> pack)
				{
					assert(NetIO::instance()->State->in_state(state::net::ToLoginServ::instance()));
					
					const auto& pre = Client::configer();
					bool unprepared = true;	// unprepared well flag 
					
					auto req = Protocol::LoginBuilder::make().deal_type("Request");
					auto game = Client::instance()->GameCore;
					if (game->AreaInfo == nullptr)
					{
						json apdx;
						apdx["Name"] = pre["RoomInfo"]["Area"].get<std::string>();
						req.deal_subtype("Area")
						   .deal_appendix(apdx);
					}
					else if (game->WeaponInfo == nullptr)
					{
						req.deal_subtype("Weapon");
					}
					// TODO: other resources 
					else
					{
						// everythng is ok
						unprepared = false;
					}

					if (unprepared)
						NetIO::instance()
						->Conn
						->channel
						->write(req.build());
				}
			}
	}},

	{ThreadId::R,
		{
			{
				"Debug", [](std::optional<ArgsPack> pack)
				{
					auto arg = std::any_cast<std::string>(pack.value()->args_pack().front());
					Render::instance()
						->submit( 
							RenderLayer::Menu,  
							"DebugShow", 
							Sprite::Updator{ [](auto sprite, auto _)
							{
								static size_t counter = 0;
								if (counter++ % 500 == 0) // continue 5s
									sprite->Age = Sprite::Forever;
							} },
							std::move(arg));
				}
			},

			{
				"InputLog", [](std::optional<ArgsPack> pack)
				{
					auto exmsg = std::any_cast<ExMessage>(pack.value()->args_pack().front());
					Render::instance()->refresh(ThreadId::U, as_str(exmsg.vkcode));
				}
			},

			{
				"NetLog", [](std::optional<ArgsPack> pack)
				{
					auto log = std::any_cast<std::string>(pack.value()->args_pack().front());
					Render::instance()->refresh(ThreadId::N, std::move(log));
				}
			},

			{
				"OfflineAlert", [](std::optional<ArgsPack> pack)
				{
					Render::instance()->refresh(ThreadId::N, "Can not connect to server, please check internet connection");
				}
			},
			
			{
				"DisplayRoomList", [](std::optional<ArgsPack> pack)
				{ 
					auto roomlist = std::any_cast<json>(pack.value()->args_pack().front());
					
					int roomcount = roomlist["Count"];
					sol::table rmarray;
					for (const auto& rm : roomlist["RoomList"])
					{
						sol::table tmp;
						// field depends on 'Create Room'
						tmp["Name"] = rm["Name"].get<std::string>();
						tmp["Area"] = rm["Area"].get<std::string>();

						rmarray.add(tmp);
					}

					Render::instance()
						->clear(RenderLayer::Menu)
						->submit(RenderLayer::Menu, "DisplayRoomList", [](auto _, auto __) {}, roomcount, std::move(rmarray))
						->refresh(ThreadId::N, roomlist.dump());
				}
			},

			{
				"DisplaySelfRoom", [](std::optional<ArgsPack> pack)
				{
					auto msg = std::any_cast<json>(pack.value()->args_pack().front());
					auto str = msg.dump();

					Render::instance()
						->clear(RenderLayer::Menu)
						->submit(RenderLayer::Menu, "DisplaySelfRoom", [](auto _, auto __) {}, str)	// TODO: change it as a table (json.decode(str))
						->refresh(ThreadId::N, std::move(str));
				}
			},

			{
				"DisplayCreateResult", [](std::optional<ArgsPack> pack)
				{
					// result: json 
					auto result = std::any_cast<json>(pack.value()->args_pack().front());
					if (not result["Result"].get<bool>()) // create room fail
					{
						std::string reason = result["Reason"].get<std::string>();
						Render::instance()->refresh(ThreadId::N, std::format("Can not create new room, reason: {}", reason));
					}
					std::cout << "Create Room result: " << result.dump() << std::endl;
				}
			},


			{
				"IsMyRound", [](std::optional<ArgsPack> pack)
				{
					Render::instance()->submit(
						RenderLayer::Active,
						"IsMyRound",
						Sprite::Updator{ [](auto sprite, auto _)
							{
								static size_t counter = 0;
								if (counter++ % 100 == 0) // continue 1s
									sprite->Age = Sprite::Forever;
							} });
				}
			},

	}},

	{ThreadId::U, {}}
};

Protocol::NetMsgResponser Protocol::LoginRpcMap =
{
	{
		"Hello",
		{
			{
				"Hello", [](const json& packet)
				{
					// set self uid field in config
					Client::configer()["Config"]["Client"]["SelfDescriptor"]["Uid"].set(packet["Uid"].get<int>());
				}
			},
		}
	},

	{
		"Request",
		{
			{
				"Room", [](const json& packet)
				{
					// notify render show room list 
					Dispatcher::dispatch(ThreadId::R, "DisplayRoomList", ArgsPackBuilder::create(packet));
				}
			},

			{
				"SelfRoom", [](const json& packet)
				{
					// update self room info
					auto& self = *(GameInfo::instance()->RoomInfo);
					self["InRoom"] = packet["Info"]["InRoom"];
					// Client::configer()["RoomInfo"]

					// notify render to show self room info  
					Dispatcher::dispatch(ThreadId::R, "DisplaySelfRoom", ArgsPackBuilder::create(packet));
				}
			},

			{
				"Area", [](const json& packet)
				{
					clog("{}", packet.dump());
					size_t realw{ packet["Width"].get<size_t>() }, realh{ packet["Height"].get<size_t>() };
					auto stream = packet["Stream"].get<std::string>();
					GameInfo::instance()->AreaInfo = new BattleArea(realw, realh, BattleArea::extract_from_bitstream(stream));
					GameInfo::instance()->update_scn_cache();


					Render::instance()->refresh(ThreadId::N, " Load Area Info Success");
				}
			},

			{
				"Weapon", [](const json& packet)
				{
					auto msgstr = packet["Weapons"].get<std::string>();
					Client::configer()["Weapon"] = Client::configer().do_string(std::move(msgstr));
					clog("{}", msgstr);
					auto myweapon = Client::configer()["Config"]["Client"]["SelfDescriptor"]["Weapon"].get<std::string>();
					GameInfo::instance()->WeaponInfo = new sol::table(Client::configer()["Weapon"][myweapon].get<sol::table>());
					
					Render::instance()->refresh(ThreadId::N, " Load Weapon Info Success");
				}
			},

		}
	},

	{
		"Order",
		{
			{
				"CreateRoom", [](const json& packet)
				{	

					Dispatcher::dispatch(ThreadId::R, "DisplayCreateResult", ArgsPackBuilder::create(packet));
					if (packet["Result"].get<bool>()) // success 
					{	   
						// record room info in configuration
						Client::configer().create_named_table("RoomInfo");
						Client::configer()["RoomInfo"] 
							= Client::configer()["Json"]["decode"]
								.call(packet["RoomInfo"].dump()).get<sol::table>();

						// record room info in game core
						GameInfo::instance()->RoomInfo = new json(packet["RoomInfo"]);
							
						// change state
						Client::instance()->State->into(state::client::InRoom::instance());
					}

					// else: stay in pick room state, do nothing   
				}
			},

			{
				"JoinRoom", [](const json& packet)
				{
					const auto& ret = packet["Appendix"];
					Render::instance()->refresh(ThreadId::N, std::format("Join room reply: {}", ret.dump()));
					// notify render show room list 
					//Dispatcher::dispatch(ThreadId::R, "DisplayJoinResult", ArgsPackBuilder::create(ret));
					//
					//if (ret["Result"].get<bool>()) {	// success 
					//	Client::instance()->State->into(state::client::InRoom::instance());
					//}
					// else: stay in pick room state  
				}
			},

			{
				"StartGame", [](const json& packet)
				{
					// change state into Battle
					Client::instance()->State->into(state::client::Battle::instance());
				}
			},
		}
	}
};

Protocol::NetMsgResponser Protocol::BattleRpcMap = 
{
	{
		"Notice",
		{
			{
				"TokenDispatch", [](const json& packet)
				{
					Client::configer()["SelfToken"].set(packet["PlaceToken"].get<std::string>());
				}
			},

			{
				"NextRound", [](const json& packet)
				{
					if (packet["PlaceToken"].get<std::string>() == Client::configer()["SelfToken"].get<std::string>())
					{
						Dispatcher::dispatch(ThreadId::R, "IsMyRound");
						UserIO::instance()
							->Mapper
							->insert_or_assign(' ', [uio = UserIO::instance()]()
							{
								clog("You hit your space to start a launch");
								uio->State->into(state::uio::GatheringPower::instance());
								NetIO::instance()->Conn->loop()->setTimeout(3 * 1000, [uio](auto _) 
									{
										uio->State->into(state::uio::InBattle::instance());
									});
							});
					}
				}
			},
		}
	},
};