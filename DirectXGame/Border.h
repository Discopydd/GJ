#pragma once
#include"KamataEngine.h"
class Border {
public:
	~Border();
	void Initialize(KamataEngine::Vector3,KamataEngine::Model*);
	void Update();
	void Draw(KamataEngine::Camera*);
	//void OnCollision();
	bool GetFlag() { return flag_; }

private:
	KamataEngine::WorldTransform worldTransform_;      
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::ObjectColor color_;
	bool flag_ = false;
};

