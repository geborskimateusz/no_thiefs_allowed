import { useEffect, useState } from 'react'
import io from 'socket.io-client'

const socket = io(window.location.origin, {
  path: '/socket.io',
  transports: ['websocket'],
})


function App() {
  const [events, setEvents] = useState([])

  useEffect(() => {
    socket.on('health_event', (data) => {
      setEvents(prev => [data.message, ...prev])
    })

    return () => socket.off('health_event')
  }, [])

  return (
    <div style={{ padding: 20 }}>
      <h1>Health Events</h1>
      <ul>
        {events.map((e, i) => (
          <li key={i}>{e}</li>
        ))}
      </ul>
    </div>
  )
}

export default App
