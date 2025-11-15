#pragma once
#include <imgui_impl_opengl3.h>
#include <vector>
#include <iostream>
#include <map>
#include <fstream>
#include "Node/Node.h"
#include <nlohmann/json.hpp>
#include <set>

class CandlestickDataManager;
struct Connection {
    std::string from_pin_guid;
    std::string to_pin_guid;
    PinOut* from_pin;
    PinIn* to_pin;
};

class BlueprintManager {
private:
    std::map<std::string, std::unique_ptr<BasicNode>> nodes;
    std::vector<Connection> connections;
    std::map<std::string, std::unique_ptr<Variable>> variables;
    CandlestickDataManager* dataManager;
    std::string blockchain;

    bool creating_connection;
    PinOut* connection_start_pin;
    bool dragging_node;
    std::string dragged_node_guid;
    ImVec2 drag_offset;
    bool FirstEntry = false;

    ImVec2 viewport_pos;
    ImVec2 viewport_size;
    std::string selected_node_guid;

    ImVec2 camera_offset;
    float camera_zoom;
    bool is_panning;
    ImVec2 pan_start_pos;
    ImVec2 pan_start_offset;

    bool show_context_menu;
    ImVec2 context_menu_pos;

    std::set<std::string> selected_nodes;
    bool is_selecting = false;
    ImVec2 selection_start;
    ImVec2 selection_end;
    bool show_selection_box = false;
    std::map<std::string, ImVec2> drag_start_positions;

    nlohmann::json clipboard_data;
    bool has_clipboard_data = false;

    std::vector<std::string> strategyFiles;
    std::string selectedStrategy;
    int selectedFileIndex;
    bool showLoadMenuPopup;
    bool showSaveMenuPopup;
    char saveFilename[256];      


public:
    BlueprintManager();

    BasicNode* getNodeByGuid(const std::string& guid);
    Pin* getPinByGuid(const std::string& guid);
    std::string GetBlockchain() { return blockchain; }
    void setBlockchain(std::string newBlockchain) { blockchain = newBlockchain; }

    void addNode(std::unique_ptr<BasicNode> node);
    void createConnection(PinOut* from_pin, PinIn* to_pin);
    void removeConnectionsToInput(PinIn* to_pin);
    void removeConnectionsFromOutput(PinOut* from_pin);
    void executeFromEntry();
    void draw();

    void saveBlueprint(const std::string& filename);
    bool loadBlueprint(const std::string& filename);
    nlohmann::json serializeToJson();
    bool deserializeFromJson(const nlohmann::json& json);
    void handleSelectionInput();
    void drawSelectionBox();
    void selectNodesInBox(ImVec2 min_pos, ImVec2 max_pos);
    void clearSelection();
    void selectNode(const std::string& guid, bool add_to_selection = false);
    void deselectNode(const std::string& guid);
    bool isNodeSelected(const std::string& guid);

    void copySelectedNodes();
    void pasteNodes(ImVec2 paste_position);
    void deleteSelectedNodes();

    void handleKeyboardInput();
    bool LoadBlueprintMenu();
    void openLoadMenu() { showLoadMenuPopup = true; }

    bool SaveBlueprintMenu();
    void renderSaveMenu();      
    void openSaveMenu() { showSaveMenuPopup = true; }
    void renderLoadMenu();

    void SetDataManager(CandlestickDataManager* dManager) { dataManager = dManager; }
    CandlestickDataManager* GetDataManager() { return dataManager; }

    void createVariable(const std::string& name, PinType type, bool isGlobal);
    void removeVariable(const std::string& guid);
    Variable* getVariable(const std::string& guid);
    const std::map<std::string, std::unique_ptr<Variable>>& getVariables() const { return variables; }

    void clearAll();
    void clearAllBeforeLoad();
private:

    void scanStrategyFiles();
    bool fileExists(const std::string& filepath);
    bool hasJsonExtension(const std::string& filename);

    void drawTopToolbar();
    void drawLeftSidebar();
    void drawViewport();

    void drawConnections();
    void drawBezierConnection(ImDrawList* draw_list, ImVec2 from, ImVec2 to, ImU32 color);
    void drawStripedBezierConnection(ImDrawList* draw_list, ImVec2 from, ImVec2 to, ImU32 primary_color, ImU32 secondary_color);
    void handleInteractions();
    void handleMouseClick(ImVec2 mouse_pos);
    void drawContextMenu();
    void drawConnectionContextMenu();
    void createNodeFromMenu(const std::string& node_type, ImVec2 world_pos);


    void handleCameraControls();
    void drawGrid(ImDrawList* draw_list);
    ImVec2 screenToWorld(ImVec2 screen_pos);
    ImVec2 worldToScreen(ImVec2 world_pos);

    void restoreConnections(const nlohmann::json& connections_json);

    BasicNode* createAutoConverter(PinType from_type, PinType to_type, ImVec2 position);
};