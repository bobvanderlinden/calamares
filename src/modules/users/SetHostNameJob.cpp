/* === This file is part of Calamares - <https://github.com/calamares> ===
 *
 *   Copyright 2014, Rohan Garg <rohan@kde.org>
 *   Copyright 2015, Teo Mrnjavac <teo@kde.org>
 *   Copyright 2018, Adriaan de Groot <groot@kde.org>
 *
 *   Calamares is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Calamares is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Calamares. If not, see <http://www.gnu.org/licenses/>.
 */

#include "SetHostNameJob.h"

#include "GlobalStorage.h"
#include "JobQueue.h"
#include "utils/CalamaresUtilsSystem.h"
#include "utils/Logger.h"

#include <QDir>
#include <QFile>

SetHostNameJob::SetHostNameJob( const QString& hostname )
    : Calamares::Job()
    , m_hostname( hostname )
{
}

QString
SetHostNameJob::prettyName() const
{
    return tr( "Set hostname %1" ).arg( m_hostname );
}


QString
SetHostNameJob::prettyDescription() const
{
    return tr( "Set hostname <strong>%1</strong>." ).arg( m_hostname );
}


QString
SetHostNameJob::prettyStatusMessage() const
{
    return tr( "Setting hostname %1." ).arg( m_hostname );
}

static bool
setFileHostname( const QString& hostname )
{
    return !( CalamaresUtils::System::instance()
                  ->createTargetFile( QStringLiteral( "/etc/hostname" ), ( hostname + '\n' ).toUtf8() )
                  .failed() );
}

static bool
writeFileEtcHosts( const QString& hostname )
{
    // The actual hostname gets substituted in at %1
    static const char etc_hosts[] = R"(# Host addresses
127.0.0.1  localhost
127.0.1.1  %1
::1        localhost ip6-localhost ip6-loopback
ff02::1    ip6-allnodes
ff02::2    ip6-allrouters
)";

    return !( CalamaresUtils::System::instance()
                  ->createTargetFile( QStringLiteral( "/etc/hosts" ), QString( etc_hosts ).arg( hostname ).toUtf8() )
                  .failed() );
}


Calamares::JobResult
SetHostNameJob::exec()
{
    Calamares::GlobalStorage* gs = Calamares::JobQueue::instance()->globalStorage();

    if ( !gs || !gs->contains( "rootMountPoint" ) )
    {
        cError() << "No rootMountPoint in global storage";
        return Calamares::JobResult::error( tr( "Internal Error" ) );
    }

    QString destDir = gs->value( "rootMountPoint" ).toString();
    if ( !QDir( destDir ).exists() )
    {
        cError() << "rootMountPoint points to a dir which does not exist";
        return Calamares::JobResult::error( tr( "Internal Error" ) );
    }

    if ( !setFileHostname( m_hostname ) )
    {
        cError() << "Can't write to hostname file";
        return Calamares::JobResult::error( tr( "Cannot write hostname to target system" ) );
    }

    if ( !writeFileEtcHosts( m_hostname ) )
    {
        cError() << "Can't write to hosts file";
        return Calamares::JobResult::error( tr( "Cannot write hostname to target system" ) );
    }

    return Calamares::JobResult::ok();
}
