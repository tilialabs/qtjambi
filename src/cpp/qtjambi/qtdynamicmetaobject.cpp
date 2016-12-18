/****************************************************************************
**
** Copyright (C) 1992-2009 Nokia. All rights reserved.
**
** This file is part of Qt Jambi.
**
** $BEGIN_LICENSE$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
** $END_LICENSE$

**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qtdynamicmetaobject.h"
#include "qtjambi_core.h"
#include "qtjambitypemanager_p.h"

#include <QtCore/QHash>
#include <QtCore/QVarLengthArray>
#include <QtCore/QMetaEnum>

class QtDynamicMetaObjectPrivate
{
    QtDynamicMetaObject *q_ptr;
    Q_DECLARE_PUBLIC(QtDynamicMetaObject);

public:
    QtDynamicMetaObjectPrivate(QtDynamicMetaObject *q, JNIEnv *env, jclass java_class, const QMetaObject *original_meta_object);
    ~QtDynamicMetaObjectPrivate();

    void initialize(JNIEnv *jni_env, jclass java_class, const QMetaObject *original_meta_object);
    void invokeMethod(JNIEnv *env, jobject object, jobject method_object, void **_a, const QString &signature = QString()) const;

    QBasicAtomicInt ref;
    int extra_data_count;

    int m_method_count;
    int m_signal_count;
    int m_property_count;

    jobjectArray m_methods;
    jobjectArray m_signals;

    jobjectArray m_property_readers;
    jobjectArray m_property_writers;
    jobjectArray m_property_resetters;
    jobjectArray m_property_designables;

    QString *m_original_signatures;
};

QtDynamicMetaObjectPrivate::QtDynamicMetaObjectPrivate(QtDynamicMetaObject *q, JNIEnv *env, jclass java_class, const QMetaObject *original_meta_object)
    : q_ptr(q), extra_data_count(0),
      m_method_count(-1), m_signal_count(0), m_property_count(0), m_methods(0), m_signals(0),
      m_property_readers(0), m_property_writers(0), m_property_resetters(0), m_property_designables(0),
      m_original_signatures(0)
{
    Q_ASSERT(env != 0);
    Q_ASSERT(REFTYPE_LOCAL(env, java_class));

    ref = 1;  /* pre c++0x compatible initialization */
    initialize(env, java_class, original_meta_object);
}

QtDynamicMetaObjectPrivate::~QtDynamicMetaObjectPrivate()
{
    JNIEnv *env = qtjambi_current_environment();
    if (env != 0) {
        if (m_methods != 0) env->DeleteGlobalRef(m_methods);
        if (m_signals != 0) env->DeleteGlobalRef(m_signals);
        if (m_property_readers != 0) env->DeleteGlobalRef(m_property_readers);
        if (m_property_writers != 0) env->DeleteGlobalRef(m_property_writers);
        if (m_property_resetters != 0) env->DeleteGlobalRef(m_property_resetters);
        if (m_property_designables != 0) env->DeleteGlobalRef(m_property_designables);
    }

    delete[] m_original_signatures;
}

void QtDynamicMetaObjectPrivate::initialize(JNIEnv *env, jclass java_class, const QMetaObject *original_meta_object)
{
    Q_Q(QtDynamicMetaObject);

    StaticCache *sc = StaticCache::instance();
    sc->resolveMetaObjectTools();

    env->PushLocalFrame(100);

    jobject meta_data_struct = env->CallStaticObjectMethod(sc->MetaObjectTools.class_ref, sc->MetaObjectTools.buildMetaData, java_class);
    qtjambi_exception_check(env);
    if (meta_data_struct == 0)
        return;

    sc->resolveMetaData();
    {
        jintArray meta_data = (jintArray) env->GetObjectField(meta_data_struct, sc->MetaData.metaData);
        Q_ASSERT(meta_data);

        jbyteArray string_data = (jbyteArray) env->GetObjectField(meta_data_struct, sc->MetaData.stringData);
        Q_ASSERT(string_data);

        {
            jclass java_superclass = env->GetSuperclass(java_class);
            q->d.superdata = qtjambi_metaobject_for_class(env, java_superclass, original_meta_object);
#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
            env->DeleteLocalRef(java_superclass);
#endif
        }
        int string_data_len = env->GetArrayLength(string_data);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
		q->d.stringdata = new QByteArrayData();
#else
        q->d.stringdata = new char[string_data_len];
#endif

        int meta_data_len = env->GetArrayLength(meta_data);
        q->d.data = new uint[meta_data_len];
        q->d.extradata = 0;

        env->GetByteArrayRegion(string_data, 0, string_data_len, (jbyte *) q->d.stringdata);
        env->GetIntArrayRegion(meta_data, 0, meta_data_len, (jint *) q->d.data);

#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
        env->DeleteLocalRef(string_data);
        env->DeleteLocalRef(meta_data);
#endif
    }

    jobjectArray lr_methods = (jobjectArray) env->GetObjectField(meta_data_struct, sc->MetaData.slotsArray);
    jobjectArray lr_signals = (jobjectArray) env->GetObjectField(meta_data_struct, sc->MetaData.signalsArray);
    jobjectArray lr_property_readers = (jobjectArray) env->GetObjectField(meta_data_struct, sc->MetaData.propertyReadersArray);
    jobjectArray lr_property_writers = (jobjectArray) env->GetObjectField(meta_data_struct, sc->MetaData.propertyWritersArray);
    jobjectArray lr_property_resetters = (jobjectArray) env->GetObjectField(meta_data_struct, sc->MetaData.propertyResettersArray);
    jobjectArray lr_property_designables = (jobjectArray) env->GetObjectField(meta_data_struct, sc->MetaData.propertyDesignablesArray);
    jobjectArray lr_extra_data = (jobjectArray) env->GetObjectField(meta_data_struct, sc->MetaData.extraDataArray);

    if (lr_methods != 0) {
        m_methods = (jobjectArray) env->NewGlobalRef(lr_methods);
#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
        env->DeleteLocalRef(lr_methods);
#endif
        m_method_count = env->GetArrayLength(m_methods);
    }

    if (lr_signals != 0) {
        m_signals = (jobjectArray) env->NewGlobalRef(lr_signals);
#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
        env->DeleteLocalRef(lr_signals);
#endif
        m_signal_count = env->GetArrayLength(m_signals);
    }

    if (m_method_count + m_signal_count > 0) {
        m_original_signatures = new QString[m_method_count + m_signal_count];
        jobjectArray original_signatures = (jobjectArray) env->GetObjectField(meta_data_struct, sc->MetaData.originalSignatures);
        for (int i=0; i<m_method_count + m_signal_count; ++i) {
            jobject lr_string = env->GetObjectArrayElement(original_signatures, i);
            m_original_signatures[i] = qtjambi_to_qstring(env, (jstring) lr_string);
#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
            env->DeleteLocalRef(lr_string);
#endif
        }
#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
        env->DeleteLocalRef(original_signatures);
#endif
    }


    if (lr_property_readers != 0) {
        m_property_readers = (jobjectArray) env->NewGlobalRef(lr_property_readers);
#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
        env->DeleteLocalRef(lr_property_readers);
#endif
        m_property_count = env->GetArrayLength(m_property_readers);
    }

    if (lr_property_writers != 0) {
        m_property_writers = (jobjectArray) env->NewGlobalRef(lr_property_writers);
#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
        env->DeleteLocalRef(lr_property_writers);
#endif
        Q_ASSERT(m_property_count == env->GetArrayLength(m_property_writers));
    }

    if (lr_property_resetters != 0) {
        m_property_resetters = (jobjectArray) env->NewGlobalRef(lr_property_resetters);
#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
        env->DeleteLocalRef(lr_property_resetters);
#endif
        Q_ASSERT(m_property_count == env->GetArrayLength(m_property_resetters));
    }

    if (lr_property_designables != 0) {
        m_property_designables = (jobjectArray) env->NewGlobalRef(lr_property_designables);
#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
        env->DeleteLocalRef(lr_property_designables);
#endif
        Q_ASSERT(m_property_count == env->GetArrayLength(m_property_designables));
    }

    if (lr_extra_data != 0) {
        extra_data_count = env->GetArrayLength(lr_extra_data);
        if (extra_data_count > 0) {
            const QMetaObject **ptr = new const QMetaObject *[extra_data_count];
            q->d.extradata = ptr;
            Q_ASSERT(q->d.extradata != 0);

            for (int i=0; i<extra_data_count; ++i) {
                jobject extra_cls = env->GetObjectArrayElement(lr_extra_data, i);
                ptr[i] = qtjambi_metaobject_for_class(env, reinterpret_cast<jclass>(extra_cls), 0);
#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
                env->DeleteLocalRef(extra_cls);
#endif
            }
        }
#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
        env->DeleteLocalRef(lr_extra_data);
#endif
    }

#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
    env->DeleteLocalRef(meta_data_struct);
#endif

    env->PopLocalFrame(0);
}

void QtDynamicMetaObjectPrivate::invokeMethod(JNIEnv *env, jobject object, jobject method_object, void **_a, const QString &_signature) const
{
    StaticCache *sc = StaticCache::instance();
    sc->resolveMetaObjectTools();

    jobject method_signature = env->CallStaticObjectMethod(sc->MetaObjectTools.class_ref, sc->MetaObjectTools.methodSignature2, method_object, true);
    Q_ASSERT(REFTYPE_LOCAL(env, method_signature));

    // If no signature is specified, we look it up
    QString signature(_signature);
    if (signature.isEmpty())
        signature = qtjambi_to_qstring(env, reinterpret_cast<jstring>(method_signature));
    Q_ASSERT(!signature.isEmpty());

#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
    env->DeleteLocalRef(method_signature);
#endif

    QtJambiTypeManager manager(env, true, QtJambiTypeManager::DynamicMetaObjectMode);

    QVector<QString> type_list = manager.parseSignature(signature);
    QVector<void *> input_arguments(type_list.size() - 1, 0);
    for (int i=1; i<type_list.size(); ++i)
        input_arguments[i - 1] = _a[i];

    QVector<void *> converted_arguments = manager.initInternalToExternal(input_arguments, type_list);
    if (converted_arguments.size() > 0) {
        QVector<jvalue> jvArgs(converted_arguments.size() - 1);
        jvalue **data = reinterpret_cast<jvalue **>(converted_arguments.data());
        for (int i=1; i<converted_arguments.count(); ++i) {
            memcpy(&jvArgs[i - 1], data[i], sizeof(jvalue));
        }

        jvalue *args = jvArgs.data();
        jvalue *returned = reinterpret_cast<jvalue *>(converted_arguments[0]);

        jvalue dummy;
        if (returned == 0) {
            dummy.j = 0;
            returned = &dummy;
        }

        jmethodID id = env->FromReflectedMethod(method_object);
        Q_ASSERT(id != 0);

        QString jni_type = QtJambiTypeManager::mangle(type_list.at(0));
        if (!jni_type.isEmpty()) {
            // TODO: Understand and audit for QTJAMBI_DEBUG_LOCALREF_CLEANUP
            switch (jni_type.at(0).toLatin1()) {
            case 'V': returned->j = 0; env->CallVoidMethodA(object, id, args); break;
            case 'I': returned->i = env->CallIntMethodA(object, id, args); break;
            case 'J': returned->j = env->CallLongMethodA(object, id, args); break;
            case 'Z': returned->z = env->CallBooleanMethodA(object, id, args); break;
            case 'S': returned->s = env->CallShortMethodA(object, id, args); break;
            case 'B': returned->b = env->CallByteMethodA(object, id, args); break;
            case 'F': returned->f = env->CallFloatMethodA(object, id, args); break;
            case 'D': returned->d = env->CallDoubleMethodA(object, id, args); break;
            case 'C': returned->c = env->CallCharMethodA(object, id, args); break;
            case 'L': returned->l = env->CallObjectMethodA(object, id, args); break;
            default:
                qWarning("QtDynamicMetaObject::invokeMethod: Unrecognized JNI type '%c'", jni_type.at(0).toLatin1());
                break;
            };
        }

        manager.convertExternalToInternal(converted_arguments.at(0), _a, type_list.at(0),
            manager.getInternalTypeName(type_list.at(0), QtJambiTypeManager::ReturnType), QtJambiTypeManager::ReturnType);

        manager.destroyConstructedExternal(converted_arguments);
    } else {
        qWarning("QtDynamicMetaObject::invokeMethod: Failed to convert arguments");
    }
}

QtDynamicMetaObject::QtDynamicMetaObject(JNIEnv *jni_env, jclass java_class, const QMetaObject *original_meta_object)
    : d_ptr(new QtDynamicMetaObjectPrivate(this, jni_env, java_class, original_meta_object))
{
    Q_ASSERT(REFTYPE_LOCAL(jni_env, java_class));
}

QtDynamicMetaObject::~QtDynamicMetaObject()
{
    {
        delete[] (uint*)d.data;
        delete[] (char*)d.stringdata;

        if (d.extradata != 0) {
            const QMetaObject ** ptr = (const QMetaObject **) d.extradata;
            for (int i = 0; i < d_ptr->extra_data_count; ++i)
                check_dynamic_deref(ptr[i]);

            delete[] (const QMetaObject **)d.extradata;
        }
    }

    delete d_ptr;
}

int QtDynamicMetaObject::originalSignalOrSlotSignature(JNIEnv *env, int _id, QString *signature) const
{
    Q_D(const QtDynamicMetaObject);

    const QMetaObject *super_class = superClass();

    if (is_dynamic(super_class)) {
        _id = static_cast<const QtDynamicMetaObject *>(super_class)->originalSignalOrSlotSignature(env, _id, signature);
    } else {
        if (_id < super_class->methodCount()) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
			QString qt_signature = QLatin1String(super_class->className()) + QLatin1String("::") + QString::fromLatin1(super_class->method(_id).methodSignature());
#else
            QString qt_signature = QLatin1String(super_class->className()) + QLatin1String("::") + QString::fromLatin1(super_class->method(_id).signature());
#endif
            *signature = getJavaName(qt_signature.toLatin1());
        }
        _id -= super_class->methodCount();
    }
    if (_id < 0) return _id;

    if (_id < d->m_signal_count + d->m_method_count)
        *signature = d->m_original_signatures[_id];

    return _id - d->m_method_count - d->m_signal_count;
}

int QtDynamicMetaObject::invokeSignalOrSlot(JNIEnv *env, jobject object, int _id, void **_a) const
{
    Q_D(const QtDynamicMetaObject);

    const QMetaObject *super_class = superClass();
    if (is_dynamic(super_class))
        _id = static_cast<const QtDynamicMetaObject *>(super_class)->invokeSignalOrSlot(env, object, _id, _a);
    if (_id < 0) return _id;

    // Emit the correct signal
    if (_id < d->m_signal_count) {
        jobject signal_field = env->GetObjectArrayElement(d->m_signals, _id);
        Q_ASSERT(signal_field);

        jfieldID field_id = env->FromReflectedField(signal_field);
        Q_ASSERT(field_id);

        jobject signal_object = env->GetObjectField(object, field_id);
        Q_ASSERT(signal_object);

        StaticCache *sc = StaticCache::instance();
        sc->resolveQtJambiInternal();

        jobject signal_emit_method = env->CallStaticObjectMethod(sc->QtJambiInternal.class_ref, sc->QtJambiInternal.findEmitMethod, signal_object);
        qtjambi_exception_check(env);
        Q_ASSERT(signal_emit_method);

        jstring j_signal_parameters = static_cast<jstring>(env->CallStaticObjectMethod(sc->QtJambiInternal.class_ref,
                                                                                       sc->QtJambiInternal.signalParameters,
                                                                                       signal_object));
        qtjambi_exception_check(env);
        Q_ASSERT(j_signal_parameters);

        // Because of type erasure, we need to find the compile time signature of the emit method
        QString signal_parameters = "void emit(" + qtjambi_to_qstring(env, j_signal_parameters) + ")";
        d->invokeMethod(env, signal_object, signal_emit_method, _a, signal_parameters);

#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
        env->DeleteLocalRef(j_signal_parameters);
        env->DeleteLocalRef(signal_emit_method);
        env->DeleteLocalRef(signal_object);
        env->DeleteLocalRef(signal_field);
#endif
    } else if (_id < d->m_signal_count + d->m_method_count) { // Call the correct method
        jobject method_object = env->GetObjectArrayElement(d->m_methods, _id - d->m_signal_count);
        Q_ASSERT(REFTYPE_LOCAL(env, method_object));

        d->invokeMethod(env, object, method_object, _a);

#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
        env->DeleteLocalRef(method_object);
#endif
    }

    return _id - d->m_method_count - d->m_signal_count;
}

int QtDynamicMetaObject::readProperty(JNIEnv *env, jobject object, int _id, void **_a) const
{
    Q_D(const QtDynamicMetaObject);

    const QMetaObject *super_class = superClass();
    if (is_dynamic(super_class))
        _id = static_cast<const QtDynamicMetaObject *>(super_class)->readProperty(env, object, _id, _a);
    if (_id < 0) return _id;

    if (_id < d->m_property_count) {
        jobject method_object = env->GetObjectArrayElement(d->m_property_readers, _id);
        Q_ASSERT(REFTYPE_LOCAL(env, method_object));

        d->invokeMethod(env, object, method_object, _a);

#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
        env->DeleteLocalRef(method_object);
#endif
    }

    return _id - d->m_property_count;
}

int QtDynamicMetaObject::writeProperty(JNIEnv *env, jobject object, int _id, void **_a) const
{
    Q_D(const QtDynamicMetaObject);

    const QMetaObject *super_class = superClass();
    if (is_dynamic(super_class))
        _id = static_cast<const QtDynamicMetaObject *>(super_class)->writeProperty(env, object, _id, _a);
    if (_id < 0) return _id;

    if (_id < d->m_property_count) {
        jobject method_object = env->GetObjectArrayElement(d->m_property_writers, _id);
        Q_ASSERT(REFTYPE_LOCAL_SAFE(env, method_object));
        if (method_object != 0) {
            // invokeMethod expects a place holder for return value, but write property meta calls
            // do not since all property writers return void by convention.
            void *a[2] = { 0, _a[0] };
            d->invokeMethod(env, object, method_object, a);

#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
            env->DeleteLocalRef(method_object);
#endif
        }
    }

    return _id - d->m_property_count;
}

int QtDynamicMetaObject::resetProperty(JNIEnv *env, jobject object, int _id, void **_a) const
{
    Q_D(const QtDynamicMetaObject);

    const QMetaObject *super_class = superClass();
    if (is_dynamic(super_class))
        _id = static_cast<const QtDynamicMetaObject *>(super_class)->resetProperty(env, object, _id, _a);
    if (_id < 0) return _id;

    if (_id < d->m_property_count) {
        jobject method_object = env->GetObjectArrayElement(d->m_property_resetters, _id);
        Q_ASSERT(REFTYPE_LOCAL_SAFE(env, method_object));
        if (method_object != 0) {
            d->invokeMethod(env, object, method_object, _a);

#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
            env->DeleteLocalRef(method_object);
#endif
        }
    }

    return _id - d->m_property_count;
}

int QtDynamicMetaObject::queryPropertyDesignable(JNIEnv *env, jobject object, int _id, void **_a) const
{
    Q_D(const QtDynamicMetaObject);

    const QMetaObject *super_class = superClass();
    if (is_dynamic(super_class))
        _id = static_cast<const QtDynamicMetaObject *>(super_class)->queryPropertyDesignable(env, object, _id, _a);
    if (_id < 0) return _id;

    if (_id < d->m_property_count) {
        jobject method_object = env->GetObjectArrayElement(d->m_property_designables, _id);
        Q_ASSERT(REFTYPE_LOCAL_SAFE(env, method_object));
        if (method_object != 0) {
            d->invokeMethod(env, object, method_object, _a);

#ifdef QTJAMBI_DEBUG_LOCALREF_CLEANUP
            env->DeleteLocalRef(method_object);
#endif
        }
    }

    return _id - d->m_property_count;
}

// Can't inline it into header unless I declare QtDynamicMetaObjectPrivate there too
bool QtDynamicMetaObject::ref() {
    bool bf = d_ptr->ref.ref();
    // Yeah there is a race here but the Q_ASSERT is still useful
    Q_ASSERT(d_ptr->ref.fetchAndAddOrdered(0) > 0);
    return bf;
}

bool QtDynamicMetaObject::deref() {
    bool bf = d_ptr->ref.deref();
    // Yeah there is a race here but the Q_ASSERT is still useful
    Q_ASSERT(d_ptr->ref.fetchAndAddOrdered(0) >= 0);
    return bf;
}

// dynamic means it was constructed and published via MetaObjectTools.java and
//  is a Java type of a subclassed QtJambiShell object.  See the initialize()
//  method above for how this happens.
bool QtDynamicMetaObject::is_dynamic(const QMetaObject *meta_object) {
    if (meta_object == 0)
        return false;

    int idx = meta_object->indexOfClassInfo("__qt__binding_shell_language");
    return (idx >= 0 && !strcmp(meta_object->classInfo(idx).value(), "Qt Jambi"));
}

// We can fix the const_cast<> use by having the shell keep a union for storage of const and non-const
//  then split up the qtjambi_metaobject_for_class into 2 parts.  Since it is a union{} we need to keep
//  call to is_dynamic() for type detection unless we store some other bitfield
//  along side the union{}.  Which maybe useful in itself to allow the method to know if it has been
//  subclassed.  For now we const_cast<>.
// meta_object maybe 0
bool QtDynamicMetaObject::check_dynamic_deref(const QMetaObject *meta_object)
{
    if(!is_dynamic(meta_object))
        return false;

    QtDynamicMetaObject *dynamic = const_cast<QtDynamicMetaObject *>(reinterpret_cast<const QtDynamicMetaObject *>(meta_object));
    if(!dynamic->deref())
        delete dynamic;

    return true;
}

// Used by QtJambiShell code to implement the dispatch
int QtDynamicMetaObject::dispatch_qt_metacall(QtJambiLink *link, const QMetaObject *meta_object, QMetaObject::Call _c, int _id, void **_a)
{
    if (!link)
        return _id;
    if (!is_dynamic(meta_object))
        return _id;
    JNIEnv *env = qtjambi_current_environment();
    if (!env)
        return _id;

    env->PushLocalFrame(100);
    const QtDynamicMetaObject *dynamic_meta_object = static_cast<const QtDynamicMetaObject *>(meta_object);
    switch (_c) {
    case QMetaObject::InvokeMetaMethod:
        _id = dynamic_meta_object->invokeSignalOrSlot(env, link->javaObject(env), _id, _a); break;
    case QMetaObject::ReadProperty:
        _id = dynamic_meta_object->readProperty(env, link->javaObject(env), _id, _a); break;
    case QMetaObject::WriteProperty:
        _id = dynamic_meta_object->writeProperty(env, link->javaObject(env), _id, _a); break;
    case QMetaObject::ResetProperty:
        _id = dynamic_meta_object->resetProperty(env, link->javaObject(env), _id, _a); break;
    case QMetaObject::QueryPropertyDesignable:
        _id = dynamic_meta_object->queryPropertyDesignable(env, link->javaObject(env), _id, _a); break;
    default:
        break;
    };
    env->PopLocalFrame(0);

    return _id;
}

const QMetaObject *QtDynamicMetaObject::build(JNIEnv *env, jobject java_object, const QMetaObject *base_meta_object)
{
    Q_ASSERT(env);
    Q_ASSERT(REFTYPE_LOCAL(env, java_object));

    jclass java_class = env->GetObjectClass(java_object);
    Q_ASSERT(java_class);

    // TODO: I don't think we can recurse, i.e. base_meta_object is not a QtDynamicMetaObject
    //  if/when we allow this we need to take a reference if we retain it.  Maybe that is
    //  what d.extradata was for.
    const QMetaObject *meta_object = qtjambi_metaobject_for_class(env, java_class, base_meta_object);
    env->DeleteLocalRef(java_class);
    return meta_object;
}
