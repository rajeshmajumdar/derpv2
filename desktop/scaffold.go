package main

import (
	"bufio"
	"fmt"
	"os"
	"path/filepath"
	"strings"
)

const (
	Reset  = "\033[0m"
	Red    = "\033[31m"
	Green  = "\033[32m"
	Yellow = "\033[33m"
	Cyan   = "\033[36m"
	Bold   = "\033[1m"
)

func main() {
	fmt.Println("\033[H\033[2J")
	fmt.Printf("%s%sderp Core%s\n", Bold, Cyan, Reset)
	fmt.Printf("Plugin Scaffolder\n\n")

	reader := bufio.NewReader(os.Stdin)

	fmt.Printf("%s?%s Enter plugin name (e.g. Inventory): ", Green, Reset)
	baseName, _ := reader.ReadString('\n')
	baseName = strings.TrimSpace(baseName)

	if baseName == "" {
		fmt.Printf("%s%sError: Plugin name cannot be empty.%s\n", Bold, Red, Reset)
		os.Exit(1)
	}

	fmt.Printf("%s?%s Enter hotkey (optional): ", Green, Reset)
	hotkey, _ := reader.ReadString('\n')
	hotkey = strings.TrimSpace(hotkey)

	fmt.Printf("%s?%s Does it require UI? [Y/n]: ", Green, Reset)
	hasUi, _ := reader.ReadString('\n')
	hasUi = strings.TrimSpace(hasUi)
	var Ui string
	if hasUi == "" || hasUi == "Y" || hasUi == "y" {
		Ui = "true"
	} else {
		Ui = "false"
	}

	fmt.Printf("%s?%s Is it exclusively for admin? [Y/n]: ", Green, Reset)
	isAdmin, _ := reader.ReadString('\n')
	isAdmin = strings.TrimSpace(isAdmin)
	if isAdmin == "" || isAdmin == "Y" || isAdmin == "y" {
		isAdmin = "master"
	} else {
		isAdmin = ""
	}

	fmt.Printf("%s?%s Enter version (e.g. 1.0.0a): ", Green, Reset)
	version, _ := reader.ReadString('\n')
	version = strings.TrimSpace(version)
	if version == "" {
		version = "1.0.0a"
	}

	fmt.Printf("%s?%s Enter the order in navbar (default: 99): ", Green, Reset)
	order, _ := reader.ReadString('\n')
	order = strings.TrimSpace(order)

	if order == "" {
		order = "99"
	}

	pluginId := strings.ReplaceAll(strings.ToLower(baseName), " ", "_")
	className := strings.ReplaceAll(baseName, " ", "") + "Plugin"
	dirName := pluginId + "_plugin"
	upperBase := strings.ToUpper(pluginId)

	pluginDir := filepath.Join("src", "plugins", dirName)

	if _, err := os.Stat(pluginDir); !os.IsNotExist(err) {
		fmt.Printf("\n%s%sError: Directory '%s' already exists. Aborting.%s\n", Bold, Red, pluginDir, Reset)
		os.Exit(1)
	}

	fmt.Printf("\n%sScaffolding %s...%s\n", Yellow, className, Reset)

	if err := os.MkdirAll(pluginDir, 0755); err != nil {
		fmt.Printf("%sError creating directory: %v%s\n", Red, err, Reset)
		os.Exit(1)
	}

	manifestContent := fmt.Sprintf(`{
		"id": "%s",
		"name": "%s",
		"version": "%s",
		"order": %s,
		"hotkey": "%s",
		"role": "%s",
		"hasUi": %s,
		"private": {},
		"chord": {},
		"intents": {},
		"permissions": {}
	}`, pluginId, baseName, version, order, hotkey, isAdmin, Ui)

	os.WriteFile(filepath.Join(pluginDir, "manifest.json"), []byte(manifestContent), 0644)
	os.WriteFile(filepath.Join(pluginDir, dirName+".json"), []byte(""), 0644)

	headerContent := fmt.Sprintf(`#ifndef %s_PLUGIN_H
#define %s_PLUGIN_H

#include "../../interfaces/DBaseModule.h"
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class %s : public DBaseModule {
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "com.derp.DModule" FILE "%s.json")
	Q_INTERFACES(DModule)

	public:
		explicit %s(QObject *parent = nullptr);
		~%s() override;

		DCore *getCore() const override { return m_core; }

		void onInitialize() override;
		void onShutdown() override;
		QWidget *createView(QWidget *parent = nullptr) override;
		void handleIntent(const QString &intent, const QVariantMap &data) override;
		void onMessage(const QString &topic, const QVariantMap &data) override;
		QVariant onServiceRequest(const QString &method, const QVariantMap &params) override;

	private:
		QWidget *m_widget = nullptr;
};

#endif // %s_PLUGIN_H
	`, upperBase, upperBase, className, dirName, className, className, upperBase)

	os.WriteFile(filepath.Join(pluginDir, className+".h"), []byte(headerContent), 0644)

	sourceContent := fmt.Sprintf(`#include "%s.h"
#include <QWidget>

%s::%s(QObject *parent)
	: DBaseModule(":/%s/manifest.json", parent) {}

%s::~%s() {
	if (m_widget && !m_widget->parent()) {
		delete m_widget;
	}
}

void %s::onInitialize() {
	if (m_core) m_core->log("[%s] Initialized.");
}

void %s::onShutdown() {
	if (m_core) m_core->log("[%s] Shutting down.");
}

QWidget *%s::createView(QWidget *parent) {
	if (!m_widget) {
		m_widget = new QWidget(parent);
	}
	return m_widget;
}

void %s::handleIntent(const QString &intent, const QVariantMap &data) {
	if (m_core) m_core->log("[%s] Intent received: " + intent);
}

void %s::onMessage(const QString &topic, const QVariantMap &data) {}

QVariant %s::onServiceRequest(const QString &method, const QVariantMap &params) {
	return QVariant();
}
	`, className, className, className, dirName, className, className, className, className, className, className, className, className, className, className, className)

	os.WriteFile(filepath.Join(pluginDir, className+".cpp"), []byte(sourceContent), 0644)

	cmakeAppend := fmt.Sprintf(`
	# ---------------------
	# %s Plugin
	# ---------------------
	set(%s_PLUGIN_DIR src/plugins/%s)

	add_library(%s MODULE
		${CMAKE_CURRENT_SOURCE_DIR}/${%s_PLUGIN_DIR}/%s.h
		${CMAKE_CURRENT_SOURCE_DIR}/${%s_PLUGIN_DIR}/%s.cpp
		src/interfaces/DBaseModule.h
	)

	target_link_libraries(%s PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui shared_widgets)

	target_include_directories(%s PRIVATE
		src/interfaces
		${CMAKE_CURRENT_SOURCE_DIR}/${%s_PLUGIN_DIR}
	)

	set_target_properties(%s PROPERTIES
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/com.derp.%s"
	)

	add_custom_command(TARGET %s POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${CMAKE_SOURCE_DIR}/${%s_PLUGIN_DIR}/manifest.json"
			"${CMAKE_BINARY_DIR}/plugins/com.derp.%s/manifest.json"
	)
	`, baseName, upperBase, dirName, dirName, upperBase, className, upperBase, className, dirName, dirName, upperBase, dirName, pluginId, dirName, upperBase, pluginId)

	f, err := os.OpenFile("CMakeLists.txt", os.O_APPEND|os.O_WRONLY|os.O_CREATE, 0644)
	if err != nil {
		fmt.Printf("%sError writing to CMakeLists.txt: %v%s\n", Red, err, Reset)
		os.Exit(1)
	}
	defer f.Close()
	f.WriteString(cmakeAppend)

	fmt.Printf("\n%s%s[+] Successfully scaffolded %s%s\n", Bold, Green, className, Reset)
	fmt.Printf("Directory: %s\n", pluginDir)
	fmt.Printf("CMake: Target injected safely.\n")
}
