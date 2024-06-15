# include <Siv3D.hpp>

/*
	よりC++ライクな書き方
	・クラスベース
	・継承を行う
*/

//==============================
// 前方宣言
//==============================
class Ball;
class Bricks;
class Paddle;

//==============================
// 定数
//==============================
namespace constants {
	namespace brick {
		/// @brief ブロックのサイズ
		constexpr Size SIZE{ 40, 20 };

		/// @brief ブロックの数　縦
		constexpr int Y_COUNT = 5;

		/// @brief ブロックの数　横
		constexpr int X_COUNT = 20;

		/// @brief 合計ブロック数
		constexpr int MAX = Y_COUNT * X_COUNT;
	}

	namespace ball {
		/// @brief ボールの速さ
		constexpr double SPEED = 480.0;

		//ボールサイズ
		constexpr int SIZE = 10;

		//ボールの初期のXポジション
		constexpr int XPOS = 400;

		//ボールの初期のYポジション
		constexpr int YPOS = 400;

		//ボールのサイズを小さくする比率
		constexpr float SHRINK_MULTIPLY = 0.95f;

		//ボールの最小サイズ
		constexpr int MIN_SIZE = 2;
	}

	namespace paddle {
		/// @brief パドルのサイズ
		constexpr Size SIZE{ 60, 10 };
	}

	namespace reflect {
		/// @brief 縦方向ベクトル
		constexpr Vec2 VERTICAL{ 1, -1 };

		/// @brief 横方向ベクトル
		constexpr Vec2 HORIZONTAL{ -1,  1 };
	}

	namespace game {
		constexpr int LIFE = 3;
	}
}

//==============================
// クラス宣言
//==============================
/// @brief ボール
class Ball final {
private:
	/// @brief 速度
	Vec2 velocity;

	/// @brief ボール
	Circle ball;

public:
	/// @brief コンストラクタ
	Ball() : velocity({ 0, -constants::ball::SPEED }), ball({ constants::ball::XPOS, constants::ball::YPOS, constants::ball::SIZE }) {}

	/// @brief デストラクタ
	~Ball() {}

	/// @brief 更新
	void Update() {
		ball.moveBy(velocity * Scene::DeltaTime());
	}

	/// @brief 描画
	void Draw() const {
		ball.draw();
	}

	Circle GetCircle() const {
		return ball;
	}

	Vec2 GetVelocity() const {
		return velocity;
	}

	/// @brief 新しい移動速度を設定
	/// @param newVelocity 新しい移動速度
	void SetVelocity(Vec2 newVelocity) {
		using namespace constants::ball;
		velocity = newVelocity.setLength(SPEED);
	}

	/// @brief 反射
	/// @param reflectVec 反射ベクトル方向
	void Reflect(const Vec2 reflectVec) {
		velocity *= reflectVec;
	}

	void ShrinkBall(float multiply) {
		using namespace constants::ball;
		//ボールの半径が2以下になったら縮小をしなくする
		if (ball.r == MIN_SIZE)return;

		auto resultR = ball.scaled(multiply).r;
		if (resultR < MIN_SIZE) {
			ball.setR(MIN_SIZE);
			return;
		}
		ball.setR(resultR);
	}

	//ボールが範囲外か
	bool IsOutOfSceneView() {
		return ball.y > Scene::Height();
	}

	//初期化
	void Init() {
		ball.setPos(constants::ball::XPOS, constants::ball::YPOS);
		velocity.set(0, -constants::ball::SPEED);
		ball.setR(constants::ball::SIZE);
	}
};

/// @brief ブロック
class Bricks final {
private:
	/// @brief ブロックリスト
	Rect brickTable[constants::brick::MAX];
	int intersectBrick = 0;
public:
	/// @brief コンストラクタ
	Bricks() {
		Init();
	}

	/// @brief デストラクタ
	~Bricks() {}

	/// @brief 衝突検知
	void Intersects(Ball* const target);

	/// @brief 描画
	void Draw() const {
		using namespace constants::brick;

		for (int i = 0; i < MAX; ++i) {
			brickTable[i].stretched(-1).draw(HSV{ brickTable[i].y - 40 });
		}
	}

	//初期化
	void Init() {
		using namespace constants::brick;
		for (int y = 0; y < Y_COUNT; ++y) {
			for (int x = 0; x < X_COUNT; ++x) {
				int index = y * X_COUNT + x;
				brickTable[index] = Rect{
					x * SIZE.x,
					60 + y * SIZE.y,
					SIZE
				};
			}
		}

		intersectBrick = 0;
	}

	//brickが存在するかどうか
	bool IsExistBrick() {
		using namespace constants::brick;
		return intersectBrick < MAX;
	}
};

/// @brief パドル
class Paddle final {
private:
	Rect paddle;

public:
	/// @brief コンストラクタ
	Paddle() : paddle(Rect(Arg::center(Cursor::Pos().x, 500), constants::paddle::SIZE)) {}

	/// @brief デストラクタ
	~Paddle() {}

	/// @brief 衝突検知
	void Intersects(Ball* const target) const;

	/// @brief 更新
	void Update() {
		paddle.x = Cursor::Pos().x - (constants::paddle::SIZE.x / 2);
	}

	/// @brief 描画
	void Draw() const {
		paddle.rounded(3).drawFrame(1, 2);
	}
};

/// @brief 壁
class Wall {
public:
	/// @brief 衝突検知
	static void Intersects(Ball* target) {
		using namespace constants;

		if (!target) {
			return;
		}

		auto velocity = target->GetVelocity();
		auto ball = target->GetCircle();

		// 天井との衝突を検知
		if ((ball.y < 0) && (velocity.y < 0))
		{
			target->Reflect(reflect::VERTICAL);
		}

		// 壁との衝突を検知
		if (((ball.x < 0) && (velocity.x < 0))
			|| ((Scene::Width() < ball.x) && (0 < velocity.x)))
		{
			target->Reflect(reflect::HORIZONTAL);
		}
	}
};

//==============================
// 定義
//==============================
void Bricks::Intersects(Ball* const target) {
	using namespace constants;
	using namespace constants::brick;

	if (!target) {
		return;
	}

	auto ball = target->GetCircle();
	for (int i = 0; i < MAX; ++i) {
		// 参照で保持
		Rect& refBrick = brickTable[i];

		// 衝突を検知
		if (refBrick.intersects(ball))
		{
			// ブロックの上辺、または底辺と交差
			if (refBrick.bottom().intersects(ball)
				|| refBrick.top().intersects(ball))
			{
				target->Reflect(reflect::VERTICAL);
			}
			else // ブロックの左辺または右辺と交差
			{
				target->Reflect(reflect::HORIZONTAL);
			}

			//あたったらボールの大きさを変える
			target->ShrinkBall(ball::SHRINK_MULTIPLY);

			//あたった回数を保持する変数をインクリメント
			intersectBrick++;

			// あたったブロックは画面外に出す
			refBrick.y -= 600;

			// 同一フレームでは複数のブロック衝突を検知しない
			break;
		}
	}
}

void Paddle::Intersects(Ball* const target) const {
	if (!target) {
		return;
	}

	auto velocity = target->GetVelocity();
	auto ball = target->GetCircle();

	if ((0 < velocity.y) && paddle.intersects(ball))
	{
		target->SetVelocity(Vec2{
			(ball.x - paddle.center().x) * 10,
			-velocity.y
		});
	}
}

enum class GameState {
	Start,
	InGame,
	GameStop,
	GameClear,
	GameEnd
};

class Game {
private:
	int life;
	GameState state = GameState::Start;
public:
	Game() {
		life = constants::game::LIFE;
	}

	//初期化
	void Init() {
		life = constants::game::LIFE;
	}

	//現在のライフ
	int GetLife() {
		return life;
	}

	//ライフを１減らす
	void DecreaseLife() {
		life--;
	}

	//生存
	bool IsExistLife() {
		return life > 0;
	}

	//ステートの変更
	void ChangeState(GameState toState) {
		state = toState;
	}

	//現在のステート
	GameState GetState() {
		return state;
	}
};
/// @brief 上下方向のグラデーションの背景を描画します。
/// @param topColor 上部の色
/// @param bottomColor 下部の色
void DrawVerticalGradientBackground(const ColorF& topColor, const ColorF& bottomColor)
{
	Scene::Rect()
		.draw(Arg::top = topColor, Arg::bottom = bottomColor);
}
//==============================
// エントリー
//==============================
void Main()
{
	Bricks bricks;
	Ball ball;
	Paddle paddle;
	Game game;

	const Font font{ FontMethod::MSDF, 48 };

	while (System::Update())
	{
		Cursor::RequestStyle(CursorStyle::Hidden);
		DrawVerticalGradientBackground(ColorF{ 0.2, 0.5, 1.0 }, Palette::Black);
		switch (game.GetState())
		{
		case GameState::Start:
			if (KeySpace.pressed())game.ChangeState(GameState::InGame);
			font(U"スペースを押してゲームスタート").drawAt(50, Scene::Width() / 2, Scene::Height() / 3);
			break;

		case GameState::InGame:
			// 更新
			paddle.Update();
			ball.Update();

			// コリジョン
			bricks.Intersects(&ball);
			Wall::Intersects(&ball);
			paddle.Intersects(&ball);


			// 描画
			bricks.Draw();
			ball.Draw();
			paddle.Draw();



			// 画面外判定
			if (ball.IsOutOfSceneView()) {
				game.DecreaseLife();
				//リスタート
				if (game.IsExistLife()) {
					game.ChangeState(GameState::GameStop);
				}
				//ゲームオーバー
				else {
					game.ChangeState(GameState::GameEnd);
					bricks.Init();
					game.Init();
				}
				ball.Init();
			}

			//ゲームクリア判定
			if (!bricks.IsExistBrick()) {
				game.ChangeState(GameState::GameClear);
				bricks.Init();
				game.Init();
				ball.Init();
			}

			break;

		case GameState::GameStop:
			if (KeySpace.pressed())game.ChangeState(GameState::InGame);
			font(U"スペースを押してリスタート").drawAt(50, Scene::Width() / 2, Scene::Height() / 3);
			font(U"残りのライフ {}"_fmt(game.GetLife())).drawAt(50, Scene::Width() / 2, Scene::Height() / 2, Palette::Yellow);
			break;

		case GameState::GameEnd:
			if (KeySpace.pressed())game.ChangeState(GameState::InGame);
			font(U"ゲームオーバー").drawAt(50, Scene::Width() / 2, Scene::Height() / 3, Palette::Red);
			font(U"スペースを押してゲームスタート").drawAt(50, Scene::Width() / 2, Scene::Height() / 2);
			break;

		case GameState::GameClear:
			if (KeySpace.pressed())game.ChangeState(GameState::InGame);
			font(U"ゲームクリア").drawAt(50, Scene::Width() / 2, Scene::Height() / 3, Palette::Yellow);
			font(U"スペースを押してゲームスタート").drawAt(50, Scene::Width() / 2, Scene::Height() / 2);
			break;
		}
	}

}


