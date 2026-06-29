namespace Gridworks.Engine;

public abstract class GwBehaviour
{
    private bool _enabled = true;

    public bool Enabled
    {
        get => _enabled;
        set
        {
            if (_enabled == value) return;
            _enabled = value;
            if (_enabled) OnEnable(); else OnDisable();
        }
    }

    public virtual void Awake()                     {}
    public virtual void Start()                     {}
    public virtual void Update(float deltaTime)     {}
    public virtual void LateUpdate(float deltaTime) {}
    public virtual void FixedUpdate()               {}
    public virtual void OnEnable()                  {}
    public virtual void OnDisable()                 {}
    public virtual void OnDestroy()                 {}
}
