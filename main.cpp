#include "./includes.h"

struct SocialButtonData {
	std::string link;
	std::string texture;
	bool isActive;
};

class SocialButton : public CCMenuItemSpriteExtra {
	std::string link;

	void onClick(CCObject*) {
		CCApplication::sharedApplication()->openURL(link.c_str());
	}

	virtual bool init(CCSprite* sprite, const std::string& link) {
		if (!CCMenuItemSpriteExtra::init(sprite, this, menu_selector(SocialButton::onClick))) return false;

		this->link = link;

		return true;
	}

public:

	static SocialButton* create(CCSprite* sprite, const std::string& link) {
		auto ret = new SocialButton();
		if (ret && ret->init(sprite, link)) {
			ret->autorelease();
			return ret;
		}
		delete ret;
		return nullptr;
	}
};

class SocialButtonsManager {

	enum DataLoadingResult {
		OK,
		FileNotFound,
		ParsingError,
		TooManyButtons
	};

	std::vector<SocialButtonData*> data;
	DataLoadingResult loadingStatus;
	static SocialButtonsManager* instance;
	
	void init() {
		loadingStatus = loadData();
	}

	DataLoadingResult loadData() {
		std::ifstream file("Resources/socialBtns.json");
		if (!file) return FileNotFound;
		std::ostringstream buffer;
		buffer << file.rdbuf();
		std::string fileContent = buffer.str();

		file.close();
		try {
			auto root = nlohmann::json::parse(fileContent);

			if (!root.contains("buttons") || !root["buttons"].is_array()) return ParsingError;

			auto buttons = root["buttons"];

			if (buttons.size() > 6) {
				return TooManyButtons;
			}

			for (const auto& btn : buttons) {
				if (!btn.contains("isBtn") || !btn["isBtn"].is_number_integer() ||
					!btn.contains("texture") || !btn["texture"].is_string() ||
					!btn.contains("link") || !btn["link"].is_string()) {
					return ParsingError;
				}

				bool isBtn = static_cast<bool>(btn["isBtn"].get<int>());
				std::string texture = btn["texture"];
				std::string link = btn["link"];

				auto buttonInfo = new SocialButtonData;
				buttonInfo->isActive = isBtn;
				buttonInfo->texture = texture;
				buttonInfo->link = link;
				data.push_back(buttonInfo);
			}
		}
		catch (...) {
			return ParsingError;
		}
		return OK;
	}

	SocialButtonsManager() {};
public:
	void onMenuLayer(MenuLayer* layer) {
		
		if (loadingStatus != OK) {

			std::string errorText;
			switch (loadingStatus){
			case SocialButtonsManager::FileNotFound:
				errorText = "Can't find 'socialBtns.json' in ./Resources";
				break;
			case SocialButtonsManager::ParsingError:
				errorText = "Can't parse 'socialBtns.json'";
				break;
			case SocialButtonsManager::TooManyButtons:
				errorText = "Too many buttons in 'socialBtns.json'";
				break;
			}

			auto size = CCDirector::sharedDirector()->getWinSize();

			auto errorLabel = CCLabelBMFont::create(errorText.c_str(), "bigFont.fnt");
			errorLabel->setColor({ 255, 0, 0 });
			errorLabel->setScale(0.6);
			errorLabel->setPosition({ size.width / 2, size.height - 10 });
			layer->addChild(errorLabel);

			return;
		}

		int childrenCount = layer->getChildrenCount();
		for (int i = 0; i < childrenCount; i++) {
			int x = (dynamic_cast<CCNode*>(layer->getChildren()->objectAtIndex(i)))->getPositionX();
			int y = (dynamic_cast<CCNode*>(layer->getChildren()->objectAtIndex(i)))->getPositionY();
			auto obj = dynamic_cast<CCNode*>(layer->getChildren()->objectAtIndex(i));

			if ((obj->getPosition().x >= 50 && obj->getPosition().x <= 51) && (obj->getPosition().y >= 24 && obj->getPosition().y <= 25)) {

				obj->removeAllChildren();
				obj->setContentSize({ 0, 0 });
				obj->setAnchorPoint({ 0, 0 });
				CCMenu* upMenu = CCMenu::create();
				CCMenu* downMenu = CCMenu::create();
				obj->addChild(upMenu);
				obj->addChild(downMenu);
				upMenu->setPosition({ 0, 30 });
				downMenu->setPosition({ 0, 0 });

				for (int j = 0; j < data.size(); j++) {
					if (!data[j]->isActive)
						continue;

					auto btnSprite = CCSprite::create(data[j]->texture.c_str());
					if (btnSprite == NULL)
						btnSprite = CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png");
					btnSprite->setScale(0.8);
					auto btn = SocialButton::create(btnSprite, data[j]->link);

					if (j < 4) upMenu->addChild(btn, 5, 100 + j);
					else downMenu->addChild(btn, 5, 100 + j);
				}
				upMenu->alignItemsHorizontallyWithPadding(5);
				downMenu->alignItemsHorizontallyWithPadding(5);
			}
		}
	}

	static SocialButtonsManager* getInstance() {
		if (!instance) {
			instance = new SocialButtonsManager();
			instance->init();
		}
		return instance;
	}

};
SocialButtonsManager* SocialButtonsManager::instance = nullptr;


bool(__thiscall* MenuLayer_init)(MenuLayer* self);
bool __fastcall MenuLayer_init_H(MenuLayer* self, void*) {
	if (!MenuLayer_init(self)) return false;
	SocialButtonsManager::getInstance()->onMenuLayer(self);
	return true;
}

void inject() {
#if _WIN32
	auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(0));

	MH_CreateHook(
		reinterpret_cast<void*>(base + 0x1907b0),
		reinterpret_cast<void*>(&MenuLayer_init_H),
		reinterpret_cast<void**>(&MenuLayer_init)
	);

	MH_EnableHook(MH_ALL_HOOKS);
#endif
}

#if _WIN32
WIN32CAC_ENTRY(inject)
#endif