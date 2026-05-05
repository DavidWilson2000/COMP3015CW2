#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <glm/glm.hpp>

// Header-only gameplay system so it can be added to the project without
// changing the Visual Studio project file. main.cpp owns rendering and UI;
// this file owns mission state, collection rules, and enemy AI behaviour.

struct HarbourMissionPart
{
    std::string name;
    glm::vec3 position;
    bool collected = false;
};

enum class HarbourEnemyState
{
    Patrol = 0,
    Chase = 1,
    ReturnToPatrol = 2
};

struct HarbourPatrolEnemy
{
    glm::vec3 position = glm::vec3(0.0f, 0.25f, 0.0f);
    glm::vec3 forward = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 lastKnownPlayerPosition = glm::vec3(0.0f);
    HarbourEnemyState state = HarbourEnemyState::Patrol;
    int waypointIndex = 0;
    float lostTimer = 0.0f;
    float catchCooldown = 0.0f;
};

enum class HarbourMissionEventType
{
    None = 0,
    PartCollected,
    Delivered,
    PlayerSpotted,
    PlayerLost,
    PlayerCaught
};

struct HarbourMissionEvent
{
    HarbourMissionEventType type = HarbourMissionEventType::None;
    std::string message;
    glm::vec3 position = glm::vec3(0.0f);
    int collectedParts = 0;
    int totalParts = 0;
};

class HarbourMission
{
public:
    HarbourMission()
    {
        Reset();
    }

    void Reset()
    {
        m_parts.clear();
        m_parts.push_back({ "Fuel Can",     glm::vec3(-17.5f, 0.35f,  10.0f), false });
        m_parts.push_back({ "Engine Coil",  glm::vec3( 16.0f, 0.35f,  19.5f), false });
        m_parts.push_back({ "Radio Battery",glm::vec3( 24.0f, 0.35f, -14.0f), false });

        m_waypoints.clear();
        m_waypoints.push_back(glm::vec3(-22.0f, 0.25f,   5.0f));
        m_waypoints.push_back(glm::vec3( -8.0f, 0.25f,  22.0f));
        m_waypoints.push_back(glm::vec3( 18.0f, 0.25f,  20.0f));
        m_waypoints.push_back(glm::vec3( 27.0f, 0.25f,  -8.0f));
        m_waypoints.push_back(glm::vec3(  5.0f, 0.25f, -24.0f));
        m_waypoints.push_back(glm::vec3(-20.0f, 0.25f, -12.0f));

        m_enemy.position = m_waypoints.empty() ? glm::vec3(-20.0f, 0.25f, 5.0f) : m_waypoints.front();
        m_enemy.forward = glm::vec3(0.0f, 0.0f, 1.0f);
        m_enemy.lastKnownPlayerPosition = m_enemy.position;
        m_enemy.state = HarbourEnemyState::Patrol;
        m_enemy.waypointIndex = 1;
        m_enemy.lostTimer = 0.0f;
        m_enemy.catchCooldown = 0.0f;

        m_complete = false;
        m_spottedLatch = false;
    }

    HarbourMissionEvent Update(float dt, const glm::vec3& playerPosition, bool interactPressed, bool atDock)
    {
        HarbourMissionEvent event;
        event.collectedParts = GetCollectedPartCount();
        event.totalParts = GetTotalPartCount();

        if (m_complete)
        {
            return event;
        }

        if (dt < 0.0f) dt = 0.0f;
        if (dt > 0.08f) dt = 0.08f;

        if (m_enemy.catchCooldown > 0.0f)
        {
            m_enemy.catchCooldown -= dt;
            if (m_enemy.catchCooldown < 0.0f) m_enemy.catchCooldown = 0.0f;
        }

        // Interact first, so the player's Q press feels responsive even while
        // the patrol is moving in the same frame.
        if (interactPressed)
        {
            if (HasAllParts() && atDock)
            {
                m_complete = true;
                event.type = HarbourMissionEventType::Delivered;
                event.message = "Boat repair parts delivered to the harbour mechanic";
                event.position = glm::vec3(0.0f, 0.35f, 0.0f);
                event.collectedParts = GetCollectedPartCount();
                event.totalParts = GetTotalPartCount();
                return event;
            }

            const int partIndex = GetNearestUncollectedPartIndex(playerPosition, m_collectRadius);
            if (partIndex >= 0 && partIndex < static_cast<int>(m_parts.size()))
            {
                m_parts[partIndex].collected = true;
                event.type = HarbourMissionEventType::PartCollected;
                event.message = "Recovered " + m_parts[partIndex].name;
                event.position = m_parts[partIndex].position;
                event.collectedParts = GetCollectedPartCount();
                event.totalParts = GetTotalPartCount();
                return event;
            }
        }

        HarbourMissionEvent aiEvent = UpdateEnemy(dt, playerPosition);
        aiEvent.collectedParts = GetCollectedPartCount();
        aiEvent.totalParts = GetTotalPartCount();
        return aiEvent;
    }

    const std::vector<HarbourMissionPart>& GetParts() const { return m_parts; }
    const HarbourPatrolEnemy& GetEnemy() const { return m_enemy; }
    bool IsComplete() const { return m_complete; }
    int GetTotalPartCount() const { return static_cast<int>(m_parts.size()); }

    int GetCollectedPartCount() const
    {
        int count = 0;
        for (const HarbourMissionPart& part : m_parts)
        {
            if (part.collected) ++count;
        }
        return count;
    }

    bool HasAllParts() const
    {
        return GetCollectedPartCount() >= GetTotalPartCount();
    }

    const char* GetEnemyStateLabel() const
    {
        if (m_enemy.state == HarbourEnemyState::Chase) return "CHASE";
        if (m_enemy.state == HarbourEnemyState::ReturnToPatrol) return "SEARCH";
        return "PATROL";
    }

    std::string GetObjectiveSummary() const
    {
        if (m_complete) return "Harbour repaired";
        if (HasAllParts()) return "Return to dock with repair parts";
        return "Repair parts " + std::to_string(GetCollectedPartCount()) + "/" + std::to_string(GetTotalPartCount());
    }

    std::string GetCompactHint(const glm::vec3& playerPosition, bool atDock) const
    {
        if (m_complete)
        {
            return "Harbour side mission complete";
        }

        if (HasAllParts() && atDock)
        {
            return "Q:DELIVER REPAIR PARTS";
        }

        const int nearbyPart = GetNearestUncollectedPartIndex(playerPosition, m_collectRadius);
        if (nearbyPart >= 0 && nearbyPart < static_cast<int>(m_parts.size()))
        {
            return "Q:SALVAGE " + m_parts[nearbyPart].name;
        }

        return "Side mission: " + GetObjectiveSummary() + " | avoid patrol | H reset";
    }

private:
    std::vector<HarbourMissionPart> m_parts;
    std::vector<glm::vec3> m_waypoints;
    HarbourPatrolEnemy m_enemy;
    bool m_complete = false;
    bool m_spottedLatch = false;

    const float m_collectRadius = 3.0f;
    const float m_detectRadius = 15.0f;
    const float m_closeDetectRadius = 4.8f;
    const float m_catchRadius = 2.0f;
    const float m_patrolSpeed = 3.4f;
    const float m_chaseSpeed = 5.8f;

    static glm::vec3 FlattenXZ(const glm::vec3& v)
    {
        return glm::vec3(v.x, 0.0f, v.z);
    }

    static float DistanceXZ(const glm::vec3& a, const glm::vec3& b)
    {
        return glm::length(FlattenXZ(a - b));
    }

    int GetNearestUncollectedPartIndex(const glm::vec3& playerPosition, float radius) const
    {
        int nearest = -1;
        float nearestDistance = radius;

        for (int i = 0; i < static_cast<int>(m_parts.size()); ++i)
        {
            if (m_parts[i].collected) continue;
            const float d = DistanceXZ(playerPosition, m_parts[i].position);
            if (d <= nearestDistance)
            {
                nearest = i;
                nearestDistance = d;
            }
        }

        return nearest;
    }

    bool CanSeePlayer(const glm::vec3& playerPosition) const
    {
        glm::vec3 toPlayer = FlattenXZ(playerPosition - m_enemy.position);
        const float distance = glm::length(toPlayer);
        if (distance <= m_closeDetectRadius) return true;
        if (distance > m_detectRadius || distance <= 0.001f) return false;

        glm::vec3 directionToPlayer = toPlayer / distance;
        glm::vec3 facing = FlattenXZ(m_enemy.forward);
        const float facingLength = glm::length(facing);
        if (facingLength <= 0.001f) facing = glm::vec3(0.0f, 0.0f, 1.0f);
        else facing /= facingLength;

        // Roughly a 120-degree vision cone. Close detection above handles
        // near misses so the enemy still feels fair and readable.
        return glm::dot(directionToPlayer, facing) > 0.50f;
    }

    bool MoveTowards(const glm::vec3& target, float speed, float dt)
    {
        glm::vec3 toTarget = FlattenXZ(target - m_enemy.position);
        const float distance = glm::length(toTarget);
        if (distance <= 0.05f)
        {
            m_enemy.position.x = target.x;
            m_enemy.position.z = target.z;
            return true;
        }

        glm::vec3 direction = toTarget / distance;
        const float step = speed * dt;
        if (step >= distance)
        {
            m_enemy.position.x = target.x;
            m_enemy.position.z = target.z;
            m_enemy.forward = direction;
            return true;
        }

        m_enemy.position += direction * step;
        m_enemy.forward = direction;
        return false;
    }

    HarbourMissionEvent UpdateEnemy(float dt, const glm::vec3& playerPosition)
    {
        HarbourMissionEvent event;
        const bool canSee = CanSeePlayer(playerPosition);

        if (canSee)
        {
            m_enemy.lastKnownPlayerPosition = playerPosition;
        }

        if (m_enemy.state == HarbourEnemyState::Patrol)
        {
            if (canSee)
            {
                m_enemy.state = HarbourEnemyState::Chase;
                m_enemy.lostTimer = 2.0f;

                if (!m_spottedLatch)
                {
                    m_spottedLatch = true;
                    event.type = HarbourMissionEventType::PlayerSpotted;
                    event.message = "Harbour patrol spotted you";
                    event.position = playerPosition;
                }
            }
            else if (!m_waypoints.empty())
            {
                const glm::vec3 target = m_waypoints[std::max(0, std::min(m_enemy.waypointIndex, static_cast<int>(m_waypoints.size()) - 1))];
                if (MoveTowards(target, m_patrolSpeed, dt))
                {
                    m_enemy.waypointIndex = (m_enemy.waypointIndex + 1) % static_cast<int>(m_waypoints.size());
                }
            }
        }
        else if (m_enemy.state == HarbourEnemyState::Chase)
        {
            if (canSee)
            {
                m_enemy.lostTimer = 2.0f;
                MoveTowards(playerPosition, m_chaseSpeed, dt);
            }
            else
            {
                m_enemy.lostTimer -= dt;
                MoveTowards(m_enemy.lastKnownPlayerPosition, m_chaseSpeed * 0.75f, dt);
                if (m_enemy.lostTimer <= 0.0f)
                {
                    m_enemy.state = HarbourEnemyState::ReturnToPatrol;
                    m_spottedLatch = false;
                    event.type = HarbourMissionEventType::PlayerLost;
                    event.message = "You lost the harbour patrol";
                    event.position = m_enemy.position;
                }
            }
        }
        else // ReturnToPatrol
        {
            if (canSee)
            {
                m_enemy.state = HarbourEnemyState::Chase;
                m_enemy.lostTimer = 2.0f;
                m_spottedLatch = true;
                event.type = HarbourMissionEventType::PlayerSpotted;
                event.message = "Harbour patrol reacquired you";
                event.position = playerPosition;
            }
            else if (!m_waypoints.empty())
            {
                const glm::vec3 target = m_waypoints[std::max(0, std::min(m_enemy.waypointIndex, static_cast<int>(m_waypoints.size()) - 1))];
                if (MoveTowards(target, m_patrolSpeed, dt))
                {
                    m_enemy.state = HarbourEnemyState::Patrol;
                    m_enemy.waypointIndex = (m_enemy.waypointIndex + 1) % static_cast<int>(m_waypoints.size());
                }
            }
        }

        if (!m_complete && m_enemy.catchCooldown <= 0.0f && DistanceXZ(playerPosition, m_enemy.position) <= m_catchRadius)
        {
            m_enemy.catchCooldown = 4.0f;
            m_enemy.state = HarbourEnemyState::ReturnToPatrol;
            m_enemy.lostTimer = 0.0f;
            m_spottedLatch = false;
            event.type = HarbourMissionEventType::PlayerCaught;
            event.message = "The patrol rammed your boat";
            event.position = playerPosition;
        }

        return event;
    }
};
