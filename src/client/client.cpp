#include "../../header/hv/hstring.h"
#include "../../header/client/client.h"

#include <sstream>

void Client::prepare_for_light()
{
    // get runtime dir
    auto cur = std::filesystem::current_path();

    auto now = datetime_now();
    clog("main start at: [{:2}:{:2}:{:2}], runtime directory:{}",
        now.hour, now.min, now.sec, cur.string());

    // load config file to setting
    auto project = cur.parent_path();
    Client::configer().require_file("Config", project.concat("\\clicfg.lua").string());
    project = project.parent_path();

    // load resource file
    load_all_mod(&Client::configer(), project.concat("\\resource\\script"));
} 

void Client::shine() noexcept
{
    this->state_->into(state::client::Prepare::instance());
    try
    {
        while (not this->state_->in_state(state::client::Wrong::instance()))
        {
            // step each state machine indivaully
            this->state_->execute();
            this->uio_->state()->execute();
            this->net_->state()->execute();
        }
    }
    catch (const std::exception &e)
    {
        clog("catch std::exception:{}", e.what());
        this->state_->into(state::client::Wrong::instance());
    }

    // program goes wrong, execute wrong state
    this->state_->execute();
}

Client *Client::lazy_init() noexcept
{
    this->state_ = new StateMachine<Client>(this);
    this->game_ = new GameInfo(); // GameInfo init in state battle
    this->render_ = Render::instance();
    this->render_->lazy_init();
    this->net_ = NetIO::instance();
    this->net_->lazy_init();
    this->uio_ = UserIO::instance();
    this->uio_->lazy_init();
    this->vm_.open_libraries(sol::lib::base, sol::lib::string, sol::lib::package, sol::lib::table, sol::lib::io);
    load_all_mod(&this->vm_, std::filesystem::path{configer()["Config"]["Client"]["ResourcePath"].get<std::string>()}.concat("\\script"));
    return this;
}

Client *Client::ensure() noexcept
{
    this->render()->ensure();
    this->netio()->ensure();
    this->uio()->ensure();
    return this;
}

Client *Client::start() noexcept
{
    this->netio()->start();
    this->uio()->start();
    this->render()->start();
    return this;
}

Client *Client::panic() noexcept
{
    this->uio()->panic();
    this->nio()->panic();
    
    // stop renderer at last
    this->render()->panic(); 
    return this;
}

Client* Client::instance()
{
    static Client cli{};
    static std::once_flag oc{};
    std::call_once(oc, [&]() { cli.lazy_init(); });
    return &cli;
}

Client *Client::i_say_there_would_be_light()
{
    return Client::instance();
}