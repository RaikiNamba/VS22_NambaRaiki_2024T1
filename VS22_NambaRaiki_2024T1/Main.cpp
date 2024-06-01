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
		//ボールの半径が2以下になったら縮小をしなくする
		if (ball.r <= 2)return;
		//ball = ball.stretched(-1 * multiply);
		ball = ball.scaled(.95f);
	}

	bool IsOutOfSceneView() {
		return ball.y > Scene::Height();
	}

	void Init() {
		ball = ball.setPos(constants::ball::XPOS, constants::ball::YPOS);
		velocity = velocity.set(0, -constants::ball::SPEED);
		ball = ball.setR(constants::ball::SIZE);
	}
};

/// @brief ブロック
class Bricks final {
private:
	/// @brief ブロックリスト
	Rect brickTable[constants::brick::MAX];

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
		paddle.rounded(3).draw();
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
				//あたったらボールの大きさを変える
				target->ShrinkBall(0.5f);
			}
			else // ブロックの左辺または右辺と交差
			{
				target->Reflect(reflect::HORIZONTAL);
				//あたったらボールの大きさを変える
				target->ShrinkBall(0.5f);
			}

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

class Game {
private:
	bool isGameStop = true;
	bool isGamimg = false;
	bool isGameOver = false;
	int life;
public:
	Game() {
		life = constants::game::LIFE;
	}

	void Init() {
		life = constants::game::LIFE;
		isGameOver = false;
	}

	void SetIsGaming(bool isGameing) {
		this->isGamimg = isGameing;
	}

	bool IsGaming() {
		return isGamimg;
	}

	bool DecreaseLife() {
		life--;
		return life > 0;
	}

	void GameOver() {
		isGameOver = true;
	}

	bool IsGameOver() {
		return isGameOver;
	}
};

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
		if (KeySpace.pressed()) {
			if (game.IsGameOver())game.Init();
			game.SetIsGaming(true);
		}

		if (game.IsGameOver()) {
			font(U"ゲームオーバー\nスペースを押してゲームスタート").draw(50, Arg::center(Scene::Width() / 2, Scene::Height() / 2));
			continue;
		}
		else if (!game.IsGaming()) {
			font(U"スペースを押してゲームスタート").draw(50, Arg::center(Scene::Width() / 2, Scene::Height() / 2));
			continue;
		}

		//==============================
		// 更新
		//==============================
		paddle.Update();
		ball.Update();

		//==============================
		// コリジョン
		//==============================
		bricks.Intersects(&ball);
		Wall::Intersects(&ball);
		paddle.Intersects(&ball);

		//==============================
		// 描画
		//==============================
		bricks.Draw();
		ball.Draw();
		paddle.Draw();

		if (ball.IsOutOfSceneView()) {
			//Restart
			if (game.DecreaseLife()) {
				Print(U"Continue");
			}
			//GameOver
			else {
				Print(U"GameOver");
				game.GameOver();
				bricks.Init();
			}
			game.SetIsGaming(false);
			ball.Init();
		}
	}
}
