using System;

namespace Gridworks.Editor;

/* Mark a class as a custom inspector panel for a specific component type.
   The editor scans loaded assemblies for this attribute and shows the
   matching panel when that component's entity is selected.

   Usage:
     [EditorCustomization(typeof(MyComponent))]
     public class MyComponentPanel : EditorPanel { ... }
*/
[AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = false)]
public sealed class EditorCustomizationAttribute : Attribute
{
    public Type TargetComponent { get; }
    public EditorCustomizationAttribute(Type targetComponent)
        => TargetComponent = targetComponent;
}

/* Base class for custom inspector panels. */
public abstract class EditorPanel
{
    /* Called every frame when the owning entity is selected in the hierarchy. */
    public abstract void OnGUI();
}