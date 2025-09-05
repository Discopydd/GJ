#include "Border.h"
#include<cassert>

Border::~Border() {}

void Border::Initialize(KamataEngine::Vector3 pos, KamataEngine::Model* model) { 
	assert(model); 
	model_ = model;
	color_.Initialize();
	worldTransform_.Initialize();
	worldTransform_.translation_ = pos;
}

void Border::Update() { 
	
	worldTransform_.TransferMatrix(); }

void Border::Draw(KamataEngine::Camera* camera) { model_->Draw(worldTransform_, *camera,&color_); }

