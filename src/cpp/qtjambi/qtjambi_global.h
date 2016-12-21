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

#ifndef QTJAMBI_GLOBAL_H
#define QTJAMBI_GLOBAL_H

#define _CRT_SECURE_NO_DEPRECATE

#include <QtCore/qglobal.h>

//TODO: rewrite
#if defined(Q_OS_WIN)
#  if !defined(QTJAMBI_EXPORT) && !defined(QT_QTJAMBI_IMPORT)
#    define QTJAMBI_EXPORT
#  elif defined(QT_QTJAMBI_IMPORT)
#    if defined(QTJAMBI_EXPORT)
#      undef QTJAMBI_EXPORT
#    endif
#    define QTJAMBI_EXPORT __declspec(dllimport)
#  elif defined(QTJAMBI_EXPORT)
#    undef QTJAMBI_EXPORT
#    define QTJAMBI_EXPORT __declspec(dllexport)
#  endif
#elif defined (QT_VISIBILITY_AVAILABLE)
#  if !defined(QTJAMBI_EXPORT) && !defined(QT_QTJAMBI_IMPORT)
#    define QTJAMBI_EXPORT
#  elif defined(QT_QTJAMBI_IMPORT)
#    if defined(QTJAMBI_EXPORT)
#      undef QTJAMBI_EXPORT
#    endif
#    define QTJAMBI_EXPORT
#  elif defined(QTJAMBI_EXPORT)
#    undef QTJAMBI_EXPORT
#    define QTJAMBI_EXPORT __attribute__((visibility("default")))
#  endif
#else
# if defined(QTJAMBI_EXPORT)
#   undef QTJAMBI_EXPORT
# endif
#  define QTJAMBI_EXPORT
#endif

#if defined Q_WS_MAC
#  include <JavaVM/jni.h>
#else
#  include <jni.h>
#endif

/* Win64 ABI does not use underscore prefixes symbols we could also use !defined(__MINGW64__) */
#if defined(Q_CC_MINGW) && !defined(_WIN64)
#  define QTJAMBI_FUNCTION_PREFIX(name) _##name
#else
#  define QTJAMBI_FUNCTION_PREFIX(name) name
#endif

typedef void (*PtrDestructorFunction)(void *);

#if defined(__unix__) || defined(__APPLE__)
    #include <pthread.h>
    #define THREAD_ID() ((void*)pthread_self())
#endif
#ifdef WIN32
    QTJAMBI_EXPORT long qtjambi_get_current_thread_id();
    #define THREAD_ID() ((void*)qtjambi_get_current_thread_id())

    #define snprintf _snprintf
#endif

QTJAMBI_EXPORT JNIEnv *qtjambi_current_environment();

#if defined(QTJAMBI_DEBUG_TOOLS) && defined(QTJAMBI_DEBUG_REFTYPE)
 #ifdef JNI_VERSION_1_6
  // This API only exists since JDK6
  #define REFTYPE_LOCAL_SAFE(env, x)       ((x) == 0 || (env)->GetObjectRefType(x) == JNILocalRefType)
  #define REFTYPE_LOCAL(env, x)            ((x) != 0 && (env)->GetObjectRefType(x) == JNILocalRefType)
  #define REFTYPE_GLOBAL_SAFE(env, x)      ((x) == 0 || (env)->GetObjectRefType(x) == JNIGlobalRefType)
  #define REFTYPE_GLOBAL(env, x)           ((x) != 0 && (env)->GetObjectRefType(x) == JNIGlobalRefType)
  #define REFTYPE_WEAKGLOBAL_SAFE(env, x)  ((x) == 0 || (env)->GetObjectRefType(x) == JNIWeakGlobalRefType)
  #define REFTYPE_WEAKGLOBAL(env, x)       ((x) != 0 && (env)->GetObjectRefType(x) == JNIWeakGlobalRefType)

  #define REFTYPE_NOENV_LOCAL_SAFE(x)      ((x) == 0 || qtjambi_current_environment()->GetObjectRefType(x) == JNILocalRefType)
  #define REFTYPE_NOENV_LOCAL(x)           ((x) != 0 && qtjambi_current_environment()->GetObjectRefType(x) == JNILocalRefType)
  #define REFTYPE_NOENV_GLOBAL_SAFE(x)     ((x) == 0 || qtjambi_current_environment()->GetObjectRefType(x) == JNIGlobalRefType)
  #define REFTYPE_NOENV_GLOBAL(x)          ((x) != 0 && qtjambi_current_environment()->GetObjectRefType(x) == JNIGlobalRefType)
  #define REFTYPE_NOENV_WEAKGLOBAL_SAFE(x) ((x) == 0 || qtjambi_current_environment()->GetObjectRefType(x) == JNIWeakGlobalRefType)
  #define REFTYPE_NOENV_WEAKGLOBAL(x)      ((x) != 0 && qtjambi_current_environment()->GetObjectRefType(x) == JNIWeakGlobalRefType)
 #else
  #define REFTYPE_LOCAL_SAFE(env, x)       (1)
  #define REFTYPE_LOCAL(env, x)            ((x) != 0)
  #define REFTYPE_GLOBAL_SAFE(env, x)      (1)
  #define REFTYPE_GLOBAL(env, x)           ((x) != 0)
  #define REFTYPE_WEAKGLOBAL_SAFE(env, x)  (1)
  #define REFTYPE_WEAKGLOBAL(env, x)       ((x) != 0)

  #define REFTYPE_NOENV_LOCAL_SAFE(x)      (1)
  #define REFTYPE_NOENV_LOCAL(x)           ((x) != 0)
  #define REFTYPE_NOENV_GLOBAL_SAFE(x)     (1)
  #define REFTYPE_NOENV_GLOBAL(x)          ((x) != 0)
  #define REFTYPE_NOENV_WEAKGLOBAL_SAFE(x) (1)
  #define REFTYPE_NOENV_WEAKGLOBAL(x)      ((x) != 0)
 #endif // JNI_VERSION_1_6
#else
 #define REFTYPE_LOCAL_SAFE(env, x)        (1)
 #define REFTYPE_LOCAL(env, x)             ((x) != 0)
 #define REFTYPE_GLOBAL_SAFE(env, x)       (1)
 #define REFTYPE_GLOBAL(env, x)            ((x) != 0)
 #define REFTYPE_WEAKGLOBAL_SAFE(env, x)   (1)
 #define REFTYPE_WEAKGLOBAL(env, x)        ((x) != 0)

 #define REFTYPE_NOENV_LOCAL_SAFE(x)       (1)
 #define REFTYPE_NOENV_LOCAL(x)            ((x) != 0)
 #define REFTYPE_NOENV_GLOBAL_SAFE(x)      (1)
 #define REFTYPE_NOENV_GLOBAL(x)           ((x) != 0)
 #define REFTYPE_NOENV_WEAKGLOBAL_SAFE(x)  (1)
 #define REFTYPE_NOENV_WEAKGLOBAL(x)       ((x) != 0)
#endif // QTJAMBI_DEBUG_TOOLS

#endif // QTJAMBI_GLOBAL_H
