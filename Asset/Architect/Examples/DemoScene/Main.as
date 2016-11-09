class Main : ScriptObject
{
    int numShots = 6;
    float firePeriod = 0.1;
    Vector3 firePosition = Vector3(0, 2, 0);
    Vector3 fireDirection = Vector3(0, 0, 1);
    float fireVelocity = 715;
    float dispersion = 0.01;
    float _elapsedTime = 0;
    int _index = 0;

    BulletSystem@ _bulletSystem;

    void Start()
    {
        _elapsedTime = 0;
        _bulletSystem = cast<BulletSystem@>(scene.GetOrCreateComponent("BulletSystem"));
    }

    void Update(float deltaTime)
    {
        _elapsedTime += deltaTime;
        while (_elapsedTime >= firePeriod)
        {
            Vector3 noise = Vector3(Random(), Random(), Random()) * 2 - Vector3(1, 1, 1);
            _elapsedTime -= _index % numShots == 0 ? firePeriod * 4 : firePeriod;
            BulletDesc bullet;
            bullet.position = firePosition;
            bullet.velocity = (fireDirection + noise * dispersion).Normalized() * fireVelocity;
            bullet.initialColor = WHITE;
            bullet.finalColor = WHITE;
            bullet.gravityFactor = 1;
            bullet.airResistance = 0.1;
            bullet.maxDistance = 2000;
            bullet.traceThickness = 0.01;
            bullet.traceLength = 20.0;
            _bulletSystem.FireBullet(bullet);
            _index++;
        }
    }
    
}

