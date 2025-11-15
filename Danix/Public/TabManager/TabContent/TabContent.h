#pragma once
#include <string>

class TabContent {
public:
    std::string name;
    bool is_active;
    bool is_visible;
    int id;

    TabContent(const std::string& _name, int _id)
        : name(_name), is_active(false), is_visible(false), id(_id) {
    }

    virtual ~TabContent() = default;
    virtual void Render() = 0;
    virtual void OnActivate() {}
    virtual void OnDeactivate() {}
    virtual void LightUpdate() {}
};