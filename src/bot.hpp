#pragma once

#include "combat.hpp"
#include "config.hpp"
#include "json.hpp"
#include "math3d.hpp"
#include "movement.hpp"
#include "tracker.hpp"
#include "world.hpp"
#include <string>

namespace mf {

class Bot {
public:
    std::string name = "ApexMech";
    int slot = 0;
    uint64_t seed = 0;
    GameConfig cfg;
    World world;
    Tracker tracker;
    CombatPlanner combat;
    MovementPlanner movement;

    double scan_yaw = 0.0;
    bool initialized = false;
    const World* shared_world = nullptr;

    const World& active_world() const {
        return shared_world ? *shared_world : world;
    }

    void init_shared(int slot_, uint64_t seed_, const GameConfig& cfg_, const World* w_) {
        slot = slot_;
        seed = seed_;
        cfg = cfg_;
        shared_world = w_;
        scan_yaw = 0.0;
        tracker = Tracker{};
        combat = CombatPlanner{};
        movement.reset();
        initialized = true;
    }

    void init(const Json& hello) {
        slot = hello["slot"].as_int(0);
        seed = static_cast<uint64_t>(hello["seed"].as_i64(0));
        cfg = GameConfig::from_json(hello["config"]);
        std::string map_dir = hello["map"]["dir"].as_str("");
        shared_world = nullptr;
        world.load(map_dir);
        scan_yaw = 0.0;
        movement.reset();
        initialized = true;
    }

    Json act(const Json& obs) {
        const World& w = active_world();
        SelfState self = Tracker::parse_self(obs["self"], cfg);
        tracker.update(obs, self, cfg, w);

        scan_yaw = wrap_angle(scan_yaw + 0.28);
        CombatDecision cd = combat.decide(obs, self, tracker, cfg, w, scan_yaw);
        MoveDecision md = movement.decide(self, tracker, cd, cfg, w);

        Json out = Json::object();
        out.set("type", "action");
        out.set("tick", obs["tick"].as_i64(0));

        Json mv = Json::array();
        mv.push(md.move.x);
        mv.push(md.move.y);
        out.set("move", std::move(mv));

        if (md.boost) out.set("boost", true);
        if (md.jump) out.set("jump", true);

        Json am = Json::array();
        am.push(cd.aim_yaw);
        am.push(clamp_val(cd.aim_pitch, self.min_pitch, self.max_pitch));
        out.set("aim", std::move(am));

        Json fr = Json::array();
        fr.push(cd.fire[0]);
        fr.push(cd.fire[1]);
        out.set("fire", std::move(fr));

        if (cd.reload) out.set("reload", true);
        if (cd.pick_upgrade >= 0) out.set("pick", cd.pick_upgrade);
        if (cd.take_pickup_id >= 0) out.set("take", cd.take_pickup_id);
        if (cd.drop_slot >= 0) out.set("drop", cd.drop_slot);

        return out;
    }
};

} // namespace mf
